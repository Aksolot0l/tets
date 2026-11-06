# Read-only executable-file diagnostic for item-render research.
# No process memory, player/world data, paths, usernames, network, or patching.
$ErrorActionPreference = 'Stop'
$expected = '2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732'
$game = Get-Process -Name 'Minecraft.Windows' -ErrorAction Stop | Select-Object -First 1
if (-not $game.Path) { throw 'Cannot find game executable path. Run this while Minecraft is open.' }
[byte[]]$data = [IO.File]::ReadAllBytes($game.Path)
$hasher = [Security.Cryptography.SHA256]::Create()
try { $hash = [BitConverter]::ToString($hasher.ComputeHash($data)).Replace('-', '').ToLowerInvariant() }
finally { $hasher.Dispose() }
if ($hash -ne $expected) { throw 'Different game build: no report was created.' }
if ($data.Length -lt 64 -or [BitConverter]::ToUInt16($data,0) -ne 0x5a4d) { throw 'Invalid executable.' }
$pe=[int][BitConverter]::ToUInt32($data,0x3c)
if ($pe -lt 0 -or $pe + 24 -gt $data.Length -or [BitConverter]::ToUInt32($data,$pe) -ne 0x4550 -or [BitConverter]::ToUInt16($data,$pe+4) -ne 0x8664) { throw 'Invalid x64 PE header.' }
$count=[int][BitConverter]::ToUInt16($data,$pe+6)
$offset=$pe+24+[int][BitConverter]::ToUInt16($data,$pe+20)
if ($count -lt 1 -or $count -gt 96 -or $offset+40*$count -gt $data.Length) { throw 'Invalid section table.' }
[uint32[]]$rvas=@(); [uint32[]]$starts=@(); [uint32[]]$sizes=@(); [uint32[]]$flags=@()
for ($i=0;$i -lt $count;$i++) {
 $o=$offset+40*$i
 $rva=[BitConverter]::ToUInt32($data,$o+12); $size=[BitConverter]::ToUInt32($data,$o+16)
 $start=[BitConverter]::ToUInt32($data,$o+20); $flag=[BitConverter]::ToUInt32($data,$o+36)
 if ([long]$start+[long]$size -gt $data.Length) { throw 'Invalid section bounds.' }
 $rvas += $rva; $starts += $start; $sizes += $size; $flags += $flag
}

# A bounded byte scanner, not a disassembler. x64 RIP-relative LEA/MOV hits
# are candidates only and do not establish renderer semantics.
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Text;
public static class ItemRenderFileScan {
  sealed class Hit { public string Name; public uint Rva; public int RefCount; }
  static byte Fold(byte c) { return (byte)(c>=65 && c<=90 ? c+32 : c); }
  static bool Match(byte[] data,int i,int end,byte[] term,bool wide) {
    int stride=wide ? 2 : 1;
    if ((long)i+(long)term.Length*stride>end) return false;
    for(int n=0;n<term.Length;n++) {
      if(Fold(data[i+n*stride])!=term[n] || (wide && data[i+n*stride+1]!=0)) return false;
    }
    return true;
  }
  public static string Scan(byte[] data,uint[] rvas,uint[] starts,uint[] sizes,uint[] flags) {
    string[] terms={"diamond_sword","iteminhandrenderer","firstpersonrenderer","helditemrenderer","itemrenderer","itemstack","renderitem","iteminhand"};
    var hits=new List<Hit>(); var result=new StringBuilder();
    result.AppendLine("Minecraft item-render executable-file scan v1");
    foreach(string term in terms) {
      byte[] q=Encoding.ASCII.GetBytes(term); int count=0;
      for(int s=0;s<rvas.Length && count<12;s++) {
        if((flags[s]&0x20000000U)!=0 || (flags[s]&0x40000000U)==0) continue;
        int begin=(int)starts[s],end=checked(begin+(int)sizes[s]);
        for(int i=begin;i<end && count<12;i++) {
          if(Fold(data[i])!=q[0]) continue;
          bool wide=i+1<end && data[i+1]==0;
          if(!Match(data,i,end,q,wide)) continue;
          uint rva=checked(rvas[s]+(uint)(i-begin));
          hits.Add(new Hit {Name=term+(wide?"/UTF16":"/ASCII"),Rva=rva}); ++count;
          result.AppendLine(String.Format("STRING {0} RVA={1:x8}",term+(wide?"/UTF16":"/ASCII"),rva));
        }
      }
      if(count==0) result.AppendLine("STRING "+term+" NOT_FOUND");
    }
    var addresses=new Dictionary<uint,List<Hit>>();
    foreach(Hit h in hits) for(int delta=-16;delta<=16;delta++) {
      long target=(long)h.Rva+delta;
      if(target<0 || target>UInt32.MaxValue) continue;
      uint key=(uint)target; List<Hit> list;
      if(!addresses.TryGetValue(key,out list)) { list=new List<Hit>(); addresses[key]=list; }
      list.Add(h);
    }
    int total=0;
    for(int s=0;s<rvas.Length && total<80;s++) {
      if((flags[s]&0x20000000U)==0) continue;
      int begin=(int)starts[s],end=checked(begin+(int)sizes[s]);
      for(int i=begin;i+7<=end && total<80;i++) {
        if((data[i]!=0x48 && data[i]!=0x4c) || (data[i+1]!=0x8d && data[i+1]!=0x8b) || (data[i+2]&0xc7)!=0x05) continue;
        uint instr=checked(rvas[s]+(uint)(i-begin));
        long target=(long)instr+7+BitConverter.ToInt32(data,i+3);
        if(target<0 || target>UInt32.MaxValue) continue;
        List<Hit> matches;
        if(!addresses.TryGetValue((uint)target,out matches)) continue;
        foreach(Hit h in matches) {
          if(h.RefCount>=8 || total>=80) continue;
          ++h.RefCount; ++total;
          result.AppendLine(String.Format("XREF {0} STRING_RVA={1:x8} TARGET_RVA={2:x8} INSTR_RVA={3:x8}",h.Name,h.Rva,target,instr));
          // Only fixed executable bytes; no surrounding data or process memory.
          int start=Math.Max(begin,i-64), length=Math.Min(256,end-start);
          uint window=checked(rvas[s]+(uint)(start-begin));
          result.AppendLine(String.Format("WINDOW RVA={0:x8} LENGTH={1:x}",window,length));
          for(int n=0;n<length;n+=32)
            result.AppendLine(BitConverter.ToString(data,start+n,Math.Min(32,length-n)).Replace("-",""));
          result.AppendLine("END");
        }
      }
    }
    result.AppendLine("TOTAL_XREFS "+total);
    return result.ToString();
  }
}
'@
$body=[ItemRenderFileScan]::Scan($data,$rvas,$starts,$sizes,$flags)
$output=Join-Path ([Environment]::GetFolderPath('Desktop')) 'ITEM_RENDER_DIAG.txt'
[IO.File]::WriteAllText($output,"EXE SHA256: $hash`r`n$body",[Text.Encoding]::ASCII)
Write-Host 'Done. Attach ITEM_RENDER_DIAG.txt from your Desktop to the chat.'
