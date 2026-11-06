# Read-only research diagnostic. Run with ONE Minecraft world open. It reads
# the PE headers and 16 bytes at each of eight published candidate code RVAs.
# It never writes to the game, hooks functions, scans memory, reads player data,
# or transmits anything. Only the executable's hash, header fields, and the
# selected machine-code bytes are printed; file paths and addresses are omitted.
$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 8) { throw 'Use 64-bit Windows PowerShell.' }
if (-not ('GayClientRenderDiagnosticV1' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;

public static class GayClientRenderDiagnosticV1 {
    [DllImport("kernel32.dll", SetLastError=true)]
    static extern IntPtr OpenProcess(uint rights, bool inherit, uint pid);
    [DllImport("kernel32.dll", SetLastError=true)]
    static extern bool ReadProcessMemory(IntPtr h, IntPtr p, byte[] b, UIntPtr n, out UIntPtr read);
    [DllImport("kernel32.dll")]
    static extern bool CloseHandle(IntPtr h);
    static byte[] Read(IntPtr h, ulong p, int n) {
        if (p < 0x10000 || p >= 0x0000800000000000UL || n <= 0 || n > 4096) return null;
        byte[] b = new byte[n]; UIntPtr got;
        return ReadProcessMemory(h, new IntPtr(unchecked((long)p)), b, new UIntPtr((uint)n), out got) && got.ToUInt64() == (ulong)n ? b : null;
    }
    static uint U32(byte[] b, int p) { return BitConverter.ToUInt32(b,p); }
    static ushort U16(byte[] b, int p) { return BitConverter.ToUInt16(b,p); }
    static string Hex(byte[] b) { return BitConverter.ToString(b).Replace("-", "").ToLowerInvariant(); }
    public static void Run() {
        Process[] games = Process.GetProcessesByName("Minecraft.Windows");
        if (games.Length != 1) { Console.WriteLine("Minecraft processes: " + games.Length + " (need exactly 1)"); return; }
        Process game = games[0];
        IntPtr h = OpenProcess(0x0410, false, (uint)game.Id); // QUERY_INFORMATION | VM_READ
        if (h == IntPtr.Zero) { Console.WriteLine("OpenProcess failed: " + Marshal.GetLastWin32Error()); return; }
        try {
            ProcessModule module;
            try { module = game.MainModule; }
            catch (Exception ex) { Console.WriteLine("Main module unavailable: " + ex.GetType().Name); return; }
            if (module == null) { Console.WriteLine("Main module unavailable"); return; }
            ulong baseAddr = unchecked((ulong)module.BaseAddress.ToInt64());
            try {
                using (FileStream file = File.OpenRead(module.FileName))
                using (SHA256 hash = SHA256.Create())
                    Console.WriteLine("EXE SHA256: " + Hex(hash.ComputeHash(file)));
            } catch (Exception ex) { Console.WriteLine("EXE SHA256 unavailable: " + ex.GetType().Name); }
            byte[] dos = Read(h,baseAddr,64);
            if (dos == null || U16(dos,0) != 0x5a4d) { Console.WriteLine("Invalid PE DOS header"); return; }
            uint ntOffset = U32(dos,0x3c);
            if (ntOffset > 0x10000) { Console.WriteLine("Invalid PE offset"); return; }
            byte[] nt = Read(h,baseAddr+ntOffset,264);
            if (nt == null || U32(nt,0) != 0x4550 || U16(nt,4) != 0x8664 || U16(nt,24) != 0x20b) {
                Console.WriteLine("Not a readable x64 PE image"); return;
            }
            ushort sections = U16(nt,6), opt = U16(nt,20);
            uint sizeImage = U32(nt,24+56);
            Console.WriteLine("PE timestamp: " + U32(nt,8).ToString("x8") + "; image size: " + sizeImage.ToString("x"));
            if (sections == 0 || sections > 96 || opt < 112 || sizeImage < 0x600000) { Console.WriteLine("Unexpected PE layout"); return; }
            byte[] table = Read(h,baseAddr+ntOffset+24+opt, sections*40);
            if (table == null) { Console.WriteLine("Cannot read section table"); return; }
            uint[] rvas = {0x581910,0x55d640,0x5ae250,0x5b07d0,0x5d0660,0x5d0890,0x5d0a90,0x5d1840};
            string[] names = {"player-render", "entity-render", "render-entities", "render-level", "tess-begin", "tess-color", "tess-vertex", "tess-draw"};
            for (int i=0;i<rvas.Length;i++) {
                uint rva = rvas[i]; bool executable = false;
                for (int s=0;s<sections;s++) {
                    int p=s*40; uint length=U32(table,p+8), va=U32(table,p+12), flags=U32(table,p+36);
                    if ((flags & 0x20000000u) != 0 && rva >= va && (ulong)rva+16 <= (ulong)va+length && (ulong)rva+16 <= sizeImage) executable=true;
                }
                byte[] bytes = executable ? Read(h,baseAddr+rva,16) : null;
                Console.WriteLine(names[i] + ": " + (bytes == null ? "unreadable-or-not-executable" : Hex(bytes)));
            }
        } finally { CloseHandle(h); }
    }
}
'@
}
[GayClientRenderDiagnosticV1]::Run()
