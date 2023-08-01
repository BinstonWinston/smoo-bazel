using System.Numerics;
using System.Runtime.InteropServices;

namespace Shared.Packet.Packets;

[Packet(PacketType.CaptureTheFlagGameState)]
public struct CaptureTheFlagGameStatePacket : IPacket {
    public byte Seconds;
    public ushort Minutes;
    public bool ResetGame;

    public short Size => 4;

    public void Serialize(Span<byte> data) {
        int offset = 0;
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<byte>())], ref Seconds);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<ushort>())], ref Minutes);
        byte resetGame = (byte)(ResetGame ? 1 : 0);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<byte>())], ref resetGame);
    }

    public void Deserialize(ReadOnlySpan<byte> data) {
        int offset = 0;
        Seconds = MemoryMarshal.Read<byte>(data[offset..(offset += Marshal.SizeOf<byte>())]);
        Minutes = MemoryMarshal.Read<ushort>(data[offset..(offset += Marshal.SizeOf<ushort>())]);
        ResetGame = MemoryMarshal.Read<byte>(data[offset..(offset += Marshal.SizeOf<byte>())]) == 1;
    }
}