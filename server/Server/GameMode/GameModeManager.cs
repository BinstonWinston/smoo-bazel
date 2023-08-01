using System.Numerics;
using System.Collections.Generic;

namespace Server.GameMode;


public class GameModeManager {
    private Thread? m_thread;
    private GameModeBase? m_gameMode;

    private static Dictionary<Guid, Vector3> playerPositions = new Dictionary<Guid, Vector3>();
    private static Dictionary<Guid, Quaternion> playerRotations = new Dictionary<Guid, Quaternion>();
    private static Dictionary<Guid, byte> playerScenarioNums = new Dictionary<Guid, byte>();
    private static Dictionary<Guid, string> playerStages = new Dictionary<Guid, string>();

    public GameModeBase? getGameMode() {
        return m_gameMode;
    }

    public void startGameMode(GameModeBase gameMode) {
        stopGameMode();
        m_gameMode = gameMode;
        m_thread = new Thread(async () => {
            await m_gameMode.run();
        });
        m_thread.Start();
    }

    public void stopGameMode() {
        if (m_gameMode != null) {
            m_gameMode.stop();
        }
        if (m_thread != null) {
            m_thread.Join();
            m_thread = null;
        }
        m_gameMode = null;
    }

    public static void updatePlayerTransform(Guid user, Vector3 pos, Quaternion rot) {
        playerPositions[user] = pos;
        playerRotations[user] = rot;
        // Console.WriteLine($"Updated player transform {user}, {pos.X},{pos.Y},{pos.Z}");
    }

    public static void updatePlayerStage(Guid user, byte scenarioNum, string stage) {
        playerScenarioNums[user] = scenarioNum;
        playerStages[user] = stage;
        Console.WriteLine($"Updated player stage {user}, {stage}:{scenarioNum}");
    }

    public static Vector3 getLastPosition(Guid user) {
        if (playerPositions.ContainsKey(user)) {
            return playerPositions[user];
        }
        return new Vector3();
    }

    public static Quaternion getLastRotation(Guid user) {
        if (playerRotations.ContainsKey(user)) {
            return playerRotations[user];
        }
        return new Quaternion();
    }

    public static byte getLastScenarioNum(Guid user) {
        if (playerScenarioNums.ContainsKey(user)) {
            return playerScenarioNums[user];
        }
        return 0;
    }

    public static string getLastStage(Guid user) {
        if (playerStages.ContainsKey(user)) {
            return playerStages[user];
        }
        return "";
    }
}