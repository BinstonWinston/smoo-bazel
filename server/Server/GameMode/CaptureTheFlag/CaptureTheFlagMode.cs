using System.Buffers;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using Shared;
using Shared.Packet;
using Shared.Packet.Packets;
using Server.GameMode;
using System.Diagnostics;
using Server;

namespace Server.GameMode.CaptureTheFlag;

public class CaptureTheFlagMode : GameMode.GameModeBase {

    private static readonly long PLAYER_TEAM_ASSIGNMENT_UPDATE_THRESHOLD_MS = 2000; // Re-send player team assignment in case clients need to restart and wouldn't otherwise have access to team assignments (without restarting the game)
    private static readonly long FLAG_PACKET_UPDATE_THRESHOLD_MS = 250; // Re-send the latest flag data if it hasn't been updated in this time threshold (e.g. the flag is on the ground and not held)
    private static readonly long ITERATION_FREQ = 30;
    private static readonly long ITERATION_TIME_THRESHOLD_MS = 1000/ITERATION_FREQ;

    private Dictionary<CaptureTheFlagTeam, FlagState> flagStates = new Dictionary<CaptureTheFlagTeam, FlagState>();
    private Dictionary<CaptureTheFlagTeam, long> lastFlagUpdateTime = new Dictionary<CaptureTheFlagTeam, long>();
    private Server m_server;
    private String[] m_redTeamNames;
    private String[] m_blueTeamNames;
    private Stopwatch m_stopwatch;
    private long m_lastPlayerTeamAssignmentUpdateTime = 0;
    private long m_lastGameTimerUpdateTime = 0;

    public CaptureTheFlagMode(String[] redTeamNames, String[] blueTeamNames, Server server) {
        this.m_server = server;
        this.m_redTeamNames = redTeamNames;
        this.m_blueTeamNames = blueTeamNames;
        flagStates[CaptureTheFlagTeam.RedTeam] = new FlagState(CaptureTheFlagTeam.RedTeam);
        flagStates[CaptureTheFlagTeam.BlueTeam] = new FlagState(CaptureTheFlagTeam.BlueTeam);
        lastFlagUpdateTime[CaptureTheFlagTeam.RedTeam] = 0;
        lastFlagUpdateTime[CaptureTheFlagTeam.BlueTeam] = 0;
        m_stopwatch = Stopwatch.StartNew();
    }

    public override async Task run() {
        await sendStartupPackets();

        m_stopwatch = Stopwatch.StartNew();
        long lastIterationTimeMs = 0;
        while (!shouldStop()) {
            await processIteration();
            var iterationTimeMs = m_stopwatch.ElapsedMilliseconds - lastIterationTimeMs;
            var waitTime = ITERATION_TIME_THRESHOLD_MS - iterationTimeMs;
            if (waitTime > 0) {
                Thread.Sleep((int)waitTime); // Subtract 2ms for noise in case the delay makes it 33 seconds
            }
            else {
                Console.WriteLine($"[Warning]: Server behind on game mode ticks. Target iteration time: {ITERATION_TIME_THRESHOLD_MS}. Actual iteration time: {iterationTimeMs}");
            }
            lastIterationTimeMs = m_stopwatch.ElapsedMilliseconds;
        }
    }

    private long getTimeSinceGameStart() {
        return m_stopwatch.ElapsedMilliseconds;
    }

    public void updateFlagData(FlagState flagState) {
        flagStates[flagState.getTeam()] = flagState;
        updateFlagStateTimestamp(flagState.getTeam());
    }

    private async Task sendStartupPackets() {
        await sendResetGamePacket();
        await sendOutPlayerTeamAssignments(true);
    }

    private async Task sendResetGamePacket() {
        // Reset the game, in case any clients already have game state (useful if the server needs to be restarted, so client state can also be reset)
        var packet = new CaptureTheFlagGameStatePacket();
        packet.Minutes = 0;
        packet.Seconds = 0;
        packet.ResetGame = true;
        await m_server.Broadcast(packet);
    }

    private async Task sendOutPlayerTeamAssignments(bool shouldCaptainHoldFlag) {
        await Parallel.ForEachAsync(m_server.Clients, async (p, _) => {
            bool isRedTeam = m_redTeamNames.Any(name => name == p.Name);
            var team = isRedTeam ? CaptureTheFlagTeam.RedTeam : CaptureTheFlagTeam.BlueTeam;
            var teamNames = isRedTeam ? m_redTeamNames : m_blueTeamNames;
            var isCaptain = p.Name == teamNames[0];
            var packet = new CaptureTheFlagPlayerTeamAssignmentPacket {
                Team = team,
                ShouldHoldTeamFlag = (shouldCaptainHoldFlag && isCaptain),
            };
            await broadcastAllWithPlayerGuid(packet, p);
        });
    }

    private async Task processIteration() {
        await processFlagStates();
        await processTeamAssignments();
        await processGameTimer();
    }

    private async Task processFlagStates() {
        foreach (var entry in flagStates) {
            var team = entry.Key;
            var flagState = entry.Value;
            var timeSinceLastUpdate = getTimeSinceGameStart() - lastFlagUpdateTime[team];
            if (timeSinceLastUpdate > FLAG_PACKET_UPDATE_THRESHOLD_MS) {
                await sendFlagPacket(flagState);
            }
        }
    }

    private async Task processTeamAssignments() {
        if (getTimeSinceGameStart() - m_lastPlayerTeamAssignmentUpdateTime > PLAYER_TEAM_ASSIGNMENT_UPDATE_THRESHOLD_MS) {
            m_lastPlayerTeamAssignmentUpdateTime = getTimeSinceGameStart();
            await sendOutPlayerTeamAssignments(false);
        }
    }

    private async Task processGameTimer() {
        if (getTimeSinceGameStart() - m_lastGameTimerUpdateTime > 1000) { // Update every 1000ms = 1s
            m_lastGameTimerUpdateTime = getTimeSinceGameStart();
            var timeSinceGameStartSeconds = getTimeSinceGameStart() / 1000;
            var packet = new CaptureTheFlagGameStatePacket();
            packet.Minutes = (ushort)(timeSinceGameStartSeconds / 60);
            packet.Seconds = (byte)(timeSinceGameStartSeconds % 60);
            packet.ResetGame = false;
            await m_server.Broadcast(packet);
        }
    }

    private void updateFlagStateTimestamp(CaptureTheFlagTeam team) {
        lastFlagUpdateTime[team] = getTimeSinceGameStart();
    }

    private async Task sendFlagPacket(FlagState state) {
        updateFlagStateTimestamp(state.getTeam());
        if (state.isHeld()) {
            return; // Don't update a held flag
        }
        var packet = new FlagDroppedPositionPacket(state);
        await m_server.Broadcast(packet);
        // Console.WriteLine($"Sent out dropped flag position packet {state.getPosition().X},{state.getPosition().Y},{state.getPosition().Z}, {state.getStage()}:{state.getScenarioNum()}");
    }

    private async Task broadcastAllWithPlayerGuid<T>(T packet, Client c) where T : struct, IPacket {
        if (!c.Connected) {
            return;
        }
        await m_server.Broadcast(packet, c);
        await c.Send(packet);
    }
}