# Read-only diagnostics for the known experimental 1.1.5 coordinate paths.
# Run while the player is in a world. Does not record coordinates, process
# memory, or addresses, write to Minecraft, or transmit data. Only status codes
# and coarse consistency checks are printed to the PowerShell window.
$ErrorActionPreference = 'Stop'
if (-not ('GayClientCoordinateDiagnosticV1' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

public static class GayClientCoordinateDiagnosticV1 {
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(uint access, bool inherit, uint pid);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool ReadProcessMemory(IntPtr process, IntPtr address, byte[] buffer, UIntPtr size, out UIntPtr read);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool CloseHandle(IntPtr handle);

    private static bool Read(IntPtr process, ulong address, byte[] value) {
        if (address == 0 || address >= 0x0000800000000000UL) return false;
        UIntPtr actual;
        return ReadProcessMemory(process, new IntPtr(unchecked((long)address)), value, new UIntPtr((uint)value.Length), out actual)
            && actual.ToUInt64() == (ulong)value.Length;
    }
    private static string Pointer(IntPtr process, ulong address, out ulong result) {
        result = 0;
        byte[] buffer = new byte[8];
        if (!Read(process, address, buffer)) return "read-failed";
        result = BitConverter.ToUInt64(buffer, 0);
        if (result == 0) return "null";
        if (result < 0x10000UL || result >= 0x0000800000000000UL) return "invalid";
        if ((result & 7) != 0) return "unaligned";
        return "ok";
    }
    private static bool Float(IntPtr process, ulong address, out float value) {
        value = 0;
        byte[] buffer = new byte[4];
        if (!Read(process, address, buffer)) return false;
        value = BitConverter.ToSingle(buffer, 0);
        return !Single.IsNaN(value) && !Single.IsInfinity(value);
    }
    private static bool Plausible(float x, float y, float z) {
        return Math.Abs(x) <= 30000000.0f && Math.Abs(y) <= 4096.0f && Math.Abs(z) <= 30000000.0f;
    }
    private static string Vector(IntPtr process, ulong address, out float x, out float y, out float z) {
        x = y = z = 0;
        if (!Float(process, address, out x) || !Float(process, address + 4, out y) || !Float(process, address + 8, out z)) return "unreadable-or-nonfinite";
        return Plausible(x, y, z) ? "plausible" : "out-of-range";
    }
    // All offsets are public, version-specific experimental paths. Each step
    // stops at the first failed read; no memory scanning is performed.
    private static bool Chain(IntPtr process, ulong start, ulong[] offsets, string name, out ulong pointer) {
        pointer = 0;
        string state = Pointer(process, start, out pointer);
        if (state != "ok") { Console.WriteLine(name + " root=" + state); return false; }
        for (int i = 0; i < offsets.Length; ++i) {
            state = Pointer(process, pointer + offsets[i], out pointer);
            if (state != "ok") { Console.WriteLine(name + " step" + (i + 1) + "=" + state); return false; }
        }
        Console.WriteLine(name + " chain=ok");
        return true;
    }
    public static void Run() {
        Process[] games = Process.GetProcessesByName("Minecraft.Windows");
        if (games.Length != 1) { Console.WriteLine("Minecraft processes=" + games.Length + "; expected exactly 1"); return; }
        IntPtr handle = OpenProcess(0x0410, false, (uint)games[0].Id); // query + read only
        if (handle == IntPtr.Zero) { Console.WriteLine("OpenProcess failed: " + Marshal.GetLastWin32Error()); return; }
        try {
            ulong baseAddress;
            try { baseAddress = unchecked((ulong)games[0].MainModule.BaseAddress.ToInt64()); }
            catch (Exception ex) { Console.WriteLine("Module lookup failed: " + ex.GetType().Name); return; }
            ulong rootA, rootB, rootC;
            string a = Pointer(handle, baseAddress + 0x019431E0, out rootA);
            Console.WriteLine("A root=" + a);
            float xA = 0, yA = 0, zA = 0, yB = 0, xC = 0, yC = 0, zC = 0;
            string aVector = "not-read", bY = "not-read", cVector = "not-read";
            if (a == "ok") {
                aVector = Vector(handle, rootA + 0x524, out xA, out yA, out zA);
                Console.WriteLine("A XYZ=" + aVector);
            }
            if (Chain(handle, baseAddress + 0x01921DF8,
                      new ulong[] {0x30,0x328,0,0x68,0x90,0x18}, "B", out rootB)) {
                bY = Float(handle, rootB + 0x164, out yB) && Math.Abs(yB) <= 4096.0f ? "plausible" : "unreadable-or-invalid";
                Console.WriteLine("B Y=" + bY);
            }
            // Alternative player-object path from the second public 1.1.5 SDK.
            if (Chain(handle, baseAddress + 0x01921DF8,
                      new ulong[] {0x30,0x2C0,0,0x78,0x28,0x58}, "C", out rootC)) {
                cVector = Vector(handle, rootC + 0x94, out xC, out yC, out zC);
                Console.WriteLine("C XYZ=" + cVector);
            }
            if (aVector == "plausible" && bY == "plausible")
                Console.WriteLine("A/B Y agreement=" + (Math.Abs(yA - yB) <= 2.5f ? "yes" : "no"));
            if (cVector == "plausible" && bY == "plausible")
                Console.WriteLine("C/B Y agreement=" + (Math.Abs(yC - yB) <= 2.5f ? "yes" : "no"));
            if (aVector == "plausible" && cVector == "plausible")
                Console.WriteLine("A/C XZ agreement=" + (Math.Abs(xA - xC) <= 2.5f && Math.Abs(zA - zC) <= 2.5f ? "yes" : "no"));
        } finally { CloseHandle(handle); }
    }
}
'@
}
[GayClientCoordinateDiagnosticV1]::Run()
