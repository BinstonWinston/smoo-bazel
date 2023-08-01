using System.Numerics;
using Shared;

namespace Shared;

public class FlagState {
    private CaptureTheFlagTeam team;
    private Guid? holder;
    private Vector3 position;
    private Quaternion rotation;
    private byte scenarioNum;
    private string stage = "";

    public FlagState(CaptureTheFlagTeam team) {
        this.team = team;
    }

    public FlagState(CaptureTheFlagTeam team, Guid? holder, Vector3 position, Quaternion rotation, byte scenarioNum, string stage) {
        this.team = team;
        this.holder = holder;
        this.position = position;
        this.rotation = rotation;
        this.scenarioNum = scenarioNum;
        this.stage = stage;
    }

    public CaptureTheFlagTeam getTeam() {
        return team;
    }

    public bool isHeld() {
        return holder != null;
    }

    public Vector3 getPosition() {
        return position;
    }

    public Quaternion getRotation() {
        return rotation;
    }

    public byte getScenarioNum() {
        return scenarioNum;
    }

    public string getStage() {
        return stage;
    }
}