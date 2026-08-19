@0xdeadbeefcafebabe;

enum MessageCode {
    workerJoin       @0;
    workerJoinAck    @1;
    workerHeartbeat  @2;
    workerLeave      @3;
    workloadSubmit   @4;
    workloadAccepted @5;
    workloadRejected @6;
    workloadStatus   @7;
    chunkAssign      @8;
    chunkAccepted    @9;
    chunkProgress    @10;
    chunkResult      @11;
    chunkFailed      @12;
    chunkCancel      @13;
    recovery         @14;
    uiSurface        @15;
    error            @16;
}

enum WorkerPlatform {
    x8664   @0;
    aarch64 @1;
    armv7   @2;
    unknown @3;
}

enum WorkerRuntime {
    posixSubproc @0;
    wasmSandbox  @1;
    container    @2;
}

enum ChunkState {
    pending   @0;
    ready     @1;
    running   @2;
    completed @3;
    failed    @4;
}

enum WorkloadState {
    submitted @0;
    planning  @1;
    executing @2;
    completed @3;
    failed    @4;
    cancelled @5;
}

enum AcceleratorType {
    none    @0;
    cuda    @1;
    opencl  @2;
    metal   @3;
    npuDsp  @4;
}

enum ThermalTier {
    nominal   @0;
    throttled @1;
    critical  @2;
}

struct WorkerCapabilities {
    platform           @0 :WorkerPlatform;
    coresOnline        @1 :UInt32;
    coresTotal         @2 :UInt32;
    ramTotalMb         @3 :UInt64;
    acceleratorType    @4 :AcceleratorType;
    acceleratorMemBytes @5 :UInt64;
    storageCapacityMb  @6 :UInt64;
    supportedRuntimes  @7 :List(WorkerRuntime);
}

struct UiSurfaceCapabilities {
    displayAttached @0 :Bool;
    displayWidth    @1 :UInt32;
    displayHeight   @2 :UInt32;
    touchSupported  @3 :Bool;
}

struct WorkerHeartbeat {
    timestampMs          @0 :UInt64;
    ramFreeMb            @1 :UInt64;
    cpuUtilizationPct    @2 :Float32;
    reservedCores        @3 :UInt32;
    reservedMemoryMb     @4 :UInt64;
    batteryPercentage    @5 :Float32;
    isCharging           @6 :Bool;
    thermalState         @7 :ThermalTier;
    networkBandwidthKbps @8 :UInt64;
    networkLatencyUs     @9 :UInt32;
    activeChunksCount    @10 :UInt32;
}

struct WorkloadDescription {
    workloadId         @0 :Text;
    sessionId          @1 :Text;
    workloadName       @2 :Text;
    totalSizeBytes     @3 :UInt64;
    priority           @4 :UInt32;
    deadlineSeconds    @5 :UInt32;
    maxRetries         @6 :UInt32;
    plannerVersion     @7 :UInt32;
}

struct WorkChunk {
    chunkId            @0 :Text;
    workloadId         @1 :Text;
    sequenceIndex      @2 :UInt32;
    dependencies       @3 :List(Text);
    requiredCpuUnits   @4 :UInt32;
    requiredMemoryMb   @5 :UInt64;
    runtimeRequired    @6 :WorkerRuntime;
    isIdempotent       @7 :Bool;
    leaseExpiresMs     @8 :UInt64;
}

struct ChunkAssign {
    chunk        @0 :WorkChunk;
    binaryPath   @1 :Text;
    inputRefPath @2 :Text;
}

struct ChunkProgress {
    chunkId            @0 :Text;
    progressPercentage @1 :Float32;
    elapsedTimeMs      @2 :UInt64;
}

struct ChunkResult {
    chunkId         @0 :Text;
    exitCode        @1 :UInt32;
    resultSha256    @2 :Text;
}

struct ChunkCancel {
    chunkId @0 :Text;
    reason  @1 :Text;
}

struct RecoveryCommand {
    failedWorkerId  @0 :Text;
    affectedLeases  @1 :List(Text);
    actionRequeue   @2 :Bool;
}

struct ErrorPayload {
    errorCode    @0 :UInt32;
    category     @1 :UInt32;
    humanDetail  @2 :Text;
}

struct ClusterMessage {
    protocolVersion @0 :UInt16;
    messageId       @1 :Text;
    correlationId   @2 :Text;
    senderId        @3 :Text;
    masterEpoch     @4 :UInt64;
    timestampMs     @5 :UInt64;
    messageCode     @6 :MessageCode;
    
    payload :union {
        joinReq       @7 :WorkerCapabilities;
        joinAck       @8 :Bool;
        heartbeat     @9 :WorkerHeartbeat;
        leaveNotify   @10 :Text;
        wlSubmit      @11 :WorkloadDescription;
        wlAccepted    @12 :Text;
        wlRejected    @13 :Text;
        wlStatus      @14 :WorkloadState;
        assignCmd     @15 :ChunkAssign;
        assignAck     @16 :Bool;
        progress      @17 :ChunkProgress;
        result        @18 :ChunkResult;
        failedNotify  @19 :ErrorPayload;
        cancelCmd     @20 :ChunkCancel;
        recoveryCmd   @21 :RecoveryCommand;
        uiState       @22 :UiSurfaceCapabilities;
        errNotify     @23 :ErrorPayload;
    }
}
