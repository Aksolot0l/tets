# Read-only executable-file diagnostic for item-render research, RTTI/vtables.
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

$optional=$pe+24
if ($optional+32 -gt $data.Length -or [BitConverter]::ToUInt16($data,$optional) -ne 0x20b) { throw 'Invalid PE32+ optional header.' }
$imageBase=[BitConverter]::ToUInt64($data,$optional+24)

# Resolve candidate MSVC x64 RTTI type descriptors, complete object locators
# and vtables in the on-disk PE. These are candidates, not a safe hook recipe.
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Text;
public static class ItemRenderRttiFileScan {
 static bool Match(byte[] b,int i,int end,byte[] q) {
  if((long)i+q.Length>end) return false;
  for(int j=0;j<q.Length;j++) if(b[i+j]!=q[j]) return false;
  return true;
 }
 static int Section(uint addr,uint[] rvas,uint[] sizes) {
  for(int i=0;i<rvas.Length;i++) if((ulong)addr>=rvas[i] && (ulong)addr<(ulong)rvas[i]+sizes[i]) return i;
  return -1;
 }
 static void Window(StringBuilder o,byte[] b,int offset,int available,uint rva) {
  int len=Math.Min(0x800,available); o.AppendLine(String.Format("CODE RVA={0:x8} LENGTH={1:x}",rva,len));
  for(int i=0;i<len;i+=32) o.AppendLine(BitConverter.ToString(b,offset+i,Math.Min(32,len-i)).Replace("-",""));
  o.AppendLine("END");
 }
 public static string Scan(byte[] b,uint[] rvas,uint[] starts,uint[] sizes,uint[] flags,ulong imageBase) {
  string[] classes={"ItemInHandRenderer","ItemRenderer"}; var output=new StringBuilder();
  output.AppendLine("Minecraft item-render RTTI file scan v2");
  var methods=new HashSet<uint>();
  int descriptors=0,locators=0,tables=0;
  foreach(string klass in classes) {
   byte[] q=Encoding.ASCII.GetBytes(klass); int found=0;
   for(int s=0;s<rvas.Length && found<8;s++) {
    if((flags[s]&0x20000000U)!=0 || (flags[s]&0x40000000U)==0) continue;
    int begin=(int)starts[s],end=checked(begin+(int)sizes[s]);
    for(int i=begin+20;i+q.Length<end && found<8;i++) {
     if(b[i]!=q[0] || !Match(b,i,end,q) || b[i+q.Length]!=(byte)'@') continue;
     // The exact class name is expected immediately after MSVC .?AV/.?AU.
     if(b[i-4]!=(byte)'.' || b[i-3]!=(byte)'?' || b[i-2]!=(byte)'A' || (b[i-1]!=(byte)'V' && b[i-1]!=(byte)'U')) continue;
     uint td=checked(rvas[s]+(uint)(i-begin-20));
     ++found; ++descriptors;
     output.AppendLine(String.Format("TYPE {0} DESCRIPTOR_RVA={1:x8} NAME_RVA={2:x8}",klass,td,td+16));
     int nCol=0;
     for(int d=0;d<rvas.Length && nCol<12;d++) {
      if((flags[d]&0x20000000U)!=0 || (flags[d]&0x40000000U)==0) continue;
      int db=(int)starts[d],de=checked(db+(int)sizes[d]);
      for(int pos=db;pos+24<=de && nCol<12;pos+=4) {
       if(BitConverter.ToUInt32(b,pos)!=1 || BitConverter.ToUInt32(b,pos+12)!=td) continue;
       uint col=checked(rvas[d]+(uint)(pos-db));
       uint self=BitConverter.ToUInt32(b,pos+20),hier=BitConverter.ToUInt32(b,pos+16);
       if(self!=col || Section(hier,rvas,sizes)<0) continue;
       ++nCol; ++locators;
       output.AppendLine(String.Format("COL {0} RVA={1:x8} OFFSET={2:x} HIERARCHY={3:x8}",klass,col,BitConverter.ToUInt32(b,pos+4),hier));
       for(int e=0;e<rvas.Length && tables<16;e++) {
        if((flags[e]&0x20000000U)!=0 || (flags[e]&0x40000000U)==0) continue;
        int eb=(int)starts[e],ee=checked(eb+(int)sizes[e]);
        for(int slot=eb;slot+16<=ee && tables<16;slot+=8) {
         if(BitConverter.ToUInt64(b,slot)!=imageBase+(ulong)col) continue;
         ++tables; uint tableRva=checked(rvas[e]+(uint)(slot-eb)+8);
         output.AppendLine(String.Format("VTABLE {0} RVA={1:x8}",klass,tableRva));
         for(int k=0;k<32 && slot+16+k*8<=ee;k++) {
          ulong ptr=BitConverter.ToUInt64(b,slot+8+k*8);
          if(ptr<imageBase || ptr-imageBase>UInt32.MaxValue) break;
          uint method=(uint)(ptr-imageBase); int section=Section(method,rvas,sizes);
          if(section<0 || (flags[section]&0x20000000U)==0) break;
          output.AppendLine(String.Format("METHOD {0} SLOT={1} RVA={2:x8}",klass,k,method));
          if(methods.Count<48 && methods.Add(method)) {
           int f=checked((int)starts[section]+(int)(method-rvas[section]));
           int available=checked((int)(sizes[section]-(method-rvas[section])));
           Window(output,b,f,available,method);
          }
         }
        }
       }
      }
     }
     if(nCol==0) output.AppendLine("NO_COL "+klass);
    }
   }
   if(found==0) output.AppendLine("NO_DESCRIPTOR "+klass);
  }
  output.AppendLine(String.Format("TOTAL TYPES={0} COL={1} VTABLES={2} METHODS={3}",descriptors,locators,tables,methods.Count));
  return output.ToString();
 }
}
'@
$body=[ItemRenderRttiFileScan]::Scan($data,$rvas,$starts,$sizes,$flags,$imageBase)
$output=Join-Path ([Environment]::GetFolderPath('Desktop')) 'ITEM_RENDER_DIAG_2.txt'
[IO.File]::WriteAllText($output,"EXE SHA256: $hash`r`n$body",[Text.Encoding]::ASCII)
Write-Host 'Done. Attach ITEM_RENDER_DIAG_2.txt from your Desktop to the chat.'
