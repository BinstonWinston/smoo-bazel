using System.Numerics;
using System.Runtime.InteropServices;

namespace Shared.Packet.Packets;

[Packet(PacketType.FlagHoldState)]
public struct FlagHoldStatePacket : IPacket {
    public short Size { get; } = 2;
    public bool IsHeld;
    public CaptureTheFlagTeam Team;

    public FlagHoldStatePacket() {
        IsHeld = false;
        Team = 0;
    }

    public void Serialize(Span<byte> data) {
        int offset = 0;
        byte isHeld = IsHeld ? (byte)1 : (byte)0;
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<byte>())], ref isHeld);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<byte>())], ref Team);
    }

    public void Deserialize(ReadOnlySpan<byte> data) {
        int offset = 0;
        IsHeld = MemoryMarshal.Read<byte>(data[offset..(offset += Marshal.SizeOf<byte>())]) == 1;
        Team = (CaptureTheFlagTeam)MemoryMarshal.Read<byte>(data[offset..(offset += Marshal.SizeOf<byte>())]);
    }
}