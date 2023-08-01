using System.Runtime.InteropServices;
using System.Text;

namespace Shared.Packet.Packets;

[Packet(PacketType.CaptureTheFlagPlayerTeamAssignment)]
public struct CaptureTheFlagPlayerTeamAssignmentPacket : IPacket {
    public CaptureTheFlagTeam Team;
    public bool ShouldHoldTeamFlag; // They start holding their team's flag

    public short Size => 2;

    static Logger consoleLogger = new Logger("Console");

    public void Serialize(Span<byte> data) {
        int offset = 0;
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<byte>())], ref Team);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<bool>())], ref ShouldHoldTeamFlag);
    }

    public void Deserialize(ReadOnlySpan<byte> data) {
        consoleLogger.Info($"Deserialized packet");
        int offset = 0;
        Team = (CaptureTheFlagTeam)MemoryMarshal.Read<byte>(data[offset..(offset += Marshal.SizeOf<byte>())]);
        ShouldHoldTeamFlag = MemoryMarshal.Read<bool>(data[offset..(offset += Marshal.SizeOf<bool>())]);
    }
}