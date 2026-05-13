declare module 'libmrp_napi.so' {
  export function init(options: {
    workDir: string;
    width: number;
    height: number;
    debug?: boolean;
  }): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function loadPackage(packagePath: string): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
  };

  export function start(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function pause(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function resume(): { ok: boolean; errorCode?: number; errorMessage?: string };
  export function sendInput(event: {
    type: 'key' | 'touch';
    action: 'down' | 'up' | 'move';
    keyCode?: number;
    x?: number;
    y?: number;
    timestamp?: number;
  }): { ok: boolean; errorCode?: number; errorMessage?: string };

  export function pullFrame(): {
    ok: boolean;
    hasNewFrame: boolean;
    exited?: boolean;
    width: number;
    height: number;
    pixelFormat: 'RGBA_8888';
    buffer?: ArrayBuffer;
    frameId?: number;
    errorCode?: number;
    errorMessage?: string;
  };

  export function release(): { ok: boolean; errorCode?: number; errorMessage?: string };

  export function inspectPureMrpPackage(path: string): {
    ok: boolean;
    errorMessage?: string;
    package?: {
      path: string;
      internalName: string;
      appName: string;
      vendor: string;
      description: string;
      appId: number;
      version: number;
      packageSize: number;
      listOffset: number;
      dataOffset: number;
      entries: Array<{
        name: string;
        offset: number;
        size: number;
        gzip: boolean;
      }>;
    };
  };

  export function probePureMythroadFs(options: {
    rootDir: string;
    packagePath?: string;
    path?: string;
    listDir?: string;
  }): {
    ok: boolean;
    errorCode?: number;
    errorMessage?: string;
    normalizedPath?: string;
    lookFor?: boolean;
    size?: {
      ok: boolean;
      size: number;
      fromPackage: boolean;
      errorMessage?: string;
    };
    list?: {
      ok: boolean;
      errorMessage?: string;
      entries: Array<{
        name: string;
        directory: boolean;
        size: number;
        fromPackage: boolean;
      }>;
    };
  };

  export function probePureMrpBackend(options: {
    workDir: string;
    packagePath: string;
    path?: string;
    listDir?: string;
    width?: number;
    height?: number;
    debug?: boolean;
  }): {
    ok: boolean;
    init: { ok: boolean; errorCode?: number; errorMessage?: string };
    load: { ok: boolean; errorCode?: number; errorMessage?: string };
    start: { ok: boolean; errorCode?: number; errorMessage?: string };
    summary: {
      workDir: string;
      packagePath: string;
      internalName: string;
      appName: string;
      vendor: string;
      description: string;
      appId: number;
      version: number;
      packageSize: number;
      entryCount: number;
      width: number;
      height: number;
      frameId: number;
      drawCount: number;
      timerInterval: number;
      apiFunctionCount: number;
      implementedApiFunctionCount: number;
      memoryBase: number;
      memoryCapacity: number;
      memoryUsed: number;
      memoryAllocationCount: number;
      loaded: boolean;
      prepared: boolean;
      exited: boolean;
    };
    apiTable: Array<{
      id: number;
      name: string;
      signature: string;
      implemented: boolean;
      legacyIndex: number;
    }>;
    apiProbe: {
      timerStart?: { ok: boolean; returnValue: number; errorMessage?: string };
      getTime?: { ok: boolean; returnValue: number; errorMessage?: string };
      timerStop?: { ok: boolean; returnValue: number; errorMessage?: string };
    };
    memoryProbe: {
      ok: boolean;
      address: number;
      size: number;
      text: string;
      errorMessage?: string;
    };
    readFileProbe: {
      ok: boolean;
      fileName: string;
      filenameAddress: number;
      lengthAddress: number;
      dataAddress: number;
      bytes: number;
      lengthValue: number;
      lookfor: number;
      head: number[];
      errorMessage?: string;
    };
    fileHandleProbe: {
      ok: boolean;
      handle: number;
      writeBytes: number;
      seekResult: number;
      readBytes: number;
      closeResult: number;
      getLenResult: number;
      renameResult: number;
      removeResult: number;
      path: string;
      renamedPath: string;
      readText: string;
      errorMessage?: string;
    };
    infoProbe: {
      ok: boolean;
      fileInfoResult: number;
      dirInfoResult: number;
      missingInfoResult: number;
      cleanupResult: number;
      filePath: string;
      dirPath: string;
      missingPath: string;
      errorMessage?: string;
    };
    screenInfoProbe: {
      ok: boolean;
      callResult: number;
      address: number;
      width: number;
      height: number;
      bit: number;
      errorMessage?: string;
    };
    userInfoProbe: {
      ok: boolean;
      callResult: number;
      address: number;
      imei: string;
      imsi: string;
      manufactory: string;
      type: string;
      version: number;
      errorMessage?: string;
    };
    datetimeProbe: {
      ok: boolean;
      callResult: number;
      address: number;
      year: number;
      month: number;
      day: number;
      hour: number;
      minute: number;
      second: number;
      errorMessage?: string;
    };
    legacyDispatchProbe: {
      ok: boolean;
      timerStartResult: number;
      getTimeResult: number;
      timerStopResult: number;
      screenInfoResult: number;
      userInfoResult: number;
      datetimeResult: number;
      drawBitmapResult: number;
      screenWidth: number;
      screenHeight: number;
      screenBit: number;
      userVersion: number;
      datetimeYear: number;
      frameId: number;
      errorMessage?: string;
    };
    directoryProbe: {
      ok: boolean;
      mkDirResult: number;
      findHandle: number;
      firstResult: number;
      nextResult: number;
      stopResult: number;
      removeFileResult: number;
      rmDirResult: number;
      path: string;
      firstEntry: string;
      nextEntry: string;
      errorMessage?: string;
    };
    startupProbe: {
      ok: boolean;
      hasStartMr: boolean;
      hasCFunctionExt: boolean;
      hasExecutableEntry: boolean;
      needsScriptVm: boolean;
      usesCFunctionBridge: boolean;
      entryName: string;
      startFile: string;
      packageArgument: string;
      runtimeKind: string;
      errorMessage?: string;
      startScripts: string[];
      mrcModules: string[];
      luaScripts: string[];
      images: string[];
      audio: string[];
      fonts: string[];
      configs: string[];
      otherResources: string[];
    };
    executorProbe: {
      ok: boolean;
      runnable: boolean;
      waitingForScriptVm: boolean;
      waitingForCFunctionBridge: boolean;
      startFileLoaded: boolean;
      startFileFromPackage: boolean;
      entryAnalyzed: boolean;
      canDispatchEvents: boolean;
      state: string;
      runtimeKind: string;
      startFile: string;
      entryFormat: string;
      packageArgument: string;
      reason?: string;
      blockedReason?: string;
      vmState: string;
      scriptVmState: string;
      scriptVmBlockedReason: string;
      lastEvent: string;
      startFileBytes: number;
      entryChecksum: number;
      entryLineCount: number;
      loadedModuleCount: number;
      loadedModuleBytes: number;
      loadedModuleChecksum: number;
      scriptGlobalCount: number;
      scriptApiFunctionCount: number;
      scriptImplementedApiFunctionCount: number;
      scriptLegacyMappedApiCount: number;
      scriptPrototypeCount: number;
      scriptInstructionCount: number;
      scriptDecodedInstructionCount: number;
      scriptUnknownOpcodeCount: number;
      scriptHasReturnOpcode: number;
      scriptConstantCount: number;
      scriptStringConstantCount: number;
      scriptChildPrototypeCount: number;
      scriptMaxStackSize: number;
      scriptStackFrameDepth: number;
      scriptStackSlotCount: number;
      scriptGlobalReadCount: number;
      scriptGlobalWriteCount: number;
      scriptRegisterCount: number;
      scriptRegisterWriteCount: number;
      scriptExecutedInstructionCount: number;
      scriptSupportedInstructionCount: number;
      scriptUnsupportedInstructionCount: number;
      scriptLoadConstantCount: number;
      scriptMoveCount: number;
      scriptGlobalOpcodeCount: number;
      scriptJumpCount: number;
      scriptCompareCount: number;
      scriptTestCount: number;
      scriptBranchSkipCount: number;
      scriptArithmeticCount: number;
      scriptUnaryCount: number;
      scriptConcatCount: number;
      scriptTableCount: number;
      scriptCallCount: number;
      scriptApiCallCount: number;
      scriptApiCallFailureCount: number;
      scriptClosureCount: number;
      scriptUpvalueCount: number;
      scriptLoopCount: number;
      scriptVarargCount: number;
      scriptClosureCallCount: number;
      scriptFrameEnterCount: number;
      scriptMaxCallDepth: number;
      scriptUpvalueBindCount: number;
      scriptOpenArgCallCount: number;
      scriptOpenReturnCount: number;
      scriptIteratorCallCount: number;
      scriptGenericLoopCount: number;
      scriptEnvironmentAccessCount: number;
      scriptStandardLibraryCallCount: number;
      scriptProtectedCallCount: number;
      scriptRegisteredModuleCount: number;
      scriptModuleLoadCount: number;
      scriptModuleCacheHitCount: number;
      scriptModuleFsLoadCount: number;
      scriptReturnReached: number;
      scriptUnsupportedOpcodeName: string;
      scriptBlockedEvents: number;
      queuedEvents: number;
      dispatchedEvents: number;
      startCount: number;
      pauseCount: number;
      resumeCount: number;
    };
    lifecycleProbe: {
      pause?: { ok: boolean; errorCode?: number; errorMessage?: string };
      resume?: { ok: boolean; errorCode?: number; errorMessage?: string };
    };
    lookFor?: boolean;
    size?: {
      ok: boolean;
      errorCode?: number;
      errorMessage?: string;
      size: number;
      fromPackage: boolean;
    };
    read?: {
      ok: boolean;
      errorCode?: number;
      errorMessage?: string;
      bytes: number;
      fromPackage: boolean;
    };
    list: {
      ok: boolean;
      errorCode?: number;
      errorMessage?: string;
      entries: Array<{
        name: string;
        directory: boolean;
        size: number;
        fromPackage: boolean;
      }>;
    };
  };
}
