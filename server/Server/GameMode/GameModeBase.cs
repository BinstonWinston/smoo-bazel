
namespace Server.GameMode;

public abstract class GameModeBase {
    private bool m_shouldStop = false;

    public abstract Task run();

    public void stop() {
        this.m_shouldStop = true;
    }

    protected bool shouldStop() {
        return this.m_shouldStop;
    }
}