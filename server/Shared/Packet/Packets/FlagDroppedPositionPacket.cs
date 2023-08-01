using System.Numerics;
using System.Runtime.InteropServices;
using Shared;
using System.Text;

namespace Shared.Packet.Packets;

[Packet(PacketType.FlagDroppedPosition)]
public struct FlagDroppedPositionPacket : IPacket {
    private static int STAGE_SIZE = 0x40;

    public CaptureTheFlagTeam Team;
    public Vector3 Position;
    public Quaternion Rotation;
    public int ScenarioNum = 0;
    public string Stage = "";

    public FlagDroppedPositionPacket(FlagState state) {
        this.Team = state.getTeam();
        this.Position = state.getPosition();
        this.Rotation = state.getRotation();
        this.ScenarioNum = state.getScenarioNum();
        this.Stage = state.getStage();
    }

    public short Size => (short)(Marshal.SizeOf<int>() + Marshal.SizeOf<Vector3>() + Marshal.SizeOf<Quaternion>() + Marshal.SizeOf<int>() + STAGE_SIZE);

    public void Serialize(Span<byte> data) {
        int offset = 0;
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<int>())], ref Team);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<Vector3>())], ref Position);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<Quaternion>())], ref Rotation);
        MemoryMarshal.Write(data[offset..(offset += Marshal.SizeOf<int>())], ref ScenarioNum);
        Encoding.UTF8.GetBytes(Stage).CopyTo(data[offset..(offset += STAGE_SIZE)]);
    }

    public void Deserialize(ReadOnlySpan<byte> data) {
        int offset = 0;
        Team = (CaptureTheFlagTeam)MemoryMarshal.Read<int>(data[offset..(offset += Marshal.SizeOf<int>())]);
        Position = MemoryMarshal.Read<Vector3>(data[offset..(offset += Marshal.SizeOf<Vector3>())]);
        Rotation = MemoryMarshal.Read<Quaternion>(data[offset..(offset += Marshal.SizeOf<Quaternion>())]);
        ScenarioNum = MemoryMarshal.Read<int>(data[offset..(offset += Marshal.SizeOf<int>())]);
        Stage = Encoding.UTF8.GetString(data[offset..(offset += STAGE_SIZE)]).TrimEnd('\0');
    }
}