using System.Numerics;
using System.Runtime.InteropServices;

namespace Shared.Packet.Packets;

[Packet(PacketType.CaptureTheFlagWin)]
public struct CaptureTheFlagWinPacket : IPacket {
    public CaptureTheFlagTeam WinningTeam;

    public short Size => 1;

    public void Serialize(Span<byte> data) {
        int offset = 0;
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<byte>())], ref WinningTeam);
    }

    public void Deserialize(ReadOnlySpan<byte> data) {
        int offset = 0;
        WinningTeam = (CaptureTheFlagTeam)MemoryMarshal.Read<byte>(data[offset..(offset += Marshal.SizeOf<byte>())]);
    }
}