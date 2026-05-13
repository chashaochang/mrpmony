import fs from '@ohos.file.fs'
import nativeMrp from 'libmrp_napi.so'
import { ensureRuntimeAssets, readBundledRawFile, runtimeWorkDir } from './RuntimeAssetBootstrap'

export function smokeTestNativeInit(): boolean {
  const result = nativeMrp.init({
    workDir: '/data/storage/el2/base/files/mrp',
    width: 240,
    height: 320,
    debug: true,
  });
  return !!result?.ok;
}

interface NativePureBackendProbeOptions {
  workDir: string
  packagePath: string
  path?: string
  listDir?: string
  width?: number
  height?: number
  debug?: boolean
}

interface NativePureBackendProbeResult {
  ok: boolean
  init?: { ok: boolean; errorCode?: number; errorMessage?: string }
  load?: { ok: boolean; errorCode?: number; errorMessage?: string }
  start?: { ok: boolean; errorCode?: number; errorMessage?: string }
  summary?: {
    appName?: string
    internalName?: string
    entryCount?: number
    prepared?: boolean
    width?: number
    height?: number
    frameId?: number
    drawCount?: number
    apiFunctionCount?: number
    implementedApiFunctionCount?: number
    memoryCapacity?: number
    memoryUsed?: number
    memoryAllocationCount?: number
  }
  apiTable?: Array<{ id: number; name: string; signature: string; implemented: boolean; legacyIndex: number }>
  apiProbe?: {
    timerStart?: { ok: boolean; returnValue: number; errorMessage?: string }
    getTime?: { ok: boolean; returnValue: number; errorMessage?: string }
    timerStop?: { ok: boolean; returnValue: number; errorMessage?: string }
  }
  memoryProbe?: { ok: boolean; address: number; size: number; text: string; errorMessage?: string }
  readFileProbe?: {
    ok: boolean
    fileName: string
    filenameAddress: number
    lengthAddress: number
    dataAddress: number
    bytes: number
    lengthValue: number
    lookfor: number
    head: number[]
    errorMessage?: string
  }
  fileHandleProbe?: {
    ok: boolean
    handle: number
    writeBytes: number
    seekResult: number
    readBytes: number
    closeResult: number
    getLenResult: number
    renameResult: number
    removeResult: number
    path: string
    renamedPath: string
    readText: string
    errorMessage?: string
  }
  infoProbe?: {
    ok: boolean
    fileInfoResult: number
    dirInfoResult: number
    missingInfoResult: number
    cleanupResult: number
    filePath: string
    dirPath: string
    missingPath: string
    errorMessage?: string
  }
  screenInfoProbe?: {
    ok: boolean
    callResult: number
    address: number
    width: number
    height: number
    bit: number
    errorMessage?: string
  }
  userInfoProbe?: {
    ok: boolean
    callResult: number
    address: number
    imei: string
    imsi: string
    manufactory: string
    type: string
    version: number
    errorMessage?: string
  }
  datetimeProbe?: {
    ok: boolean
    callResult: number
    address: number
    year: number
    month: number
    day: number
    hour: number
    minute: number
    second: number
    errorMessage?: string
  }
  legacyDispatchProbe?: {
    ok: boolean
    timerStartResult: number
    getTimeResult: number
    timerStopResult: number
    screenInfoResult: number
    userInfoResult: number
    datetimeResult: number
    drawBitmapResult: number
    screenWidth: number
    screenHeight: number
    screenBit: number
    userVersion: number
    datetimeYear: number
    frameId: number
    errorMessage?: string
  }
  directoryProbe?: {
    ok: boolean
    mkDirResult: number
    findHandle: number
    firstResult: number
    nextResult: number
    stopResult: number
    removeFileResult: number
    rmDirResult: number
    path: string
    firstEntry: string
    nextEntry: string
    errorMessage?: string
  }
  startupProbe?: {
    ok: boolean
    hasStartMr: boolean
    hasCFunctionExt: boolean
    hasExecutableEntry: boolean
    needsScriptVm: boolean
    usesCFunctionBridge: boolean
    entryName: string
    startFile: string
    packageArgument: string
    runtimeKind: string
    errorMessage?: string
    startScripts: string[]
    mrcModules: string[]
    luaScripts: string[]
    images: string[]
    audio: string[]
    fonts: string[]
    configs: string[]
    otherResources: string[]
  }
  executorProbe?: {
    ok: boolean
    runnable: boolean
    waitingForScriptVm: boolean
    waitingForCFunctionBridge: boolean
    startFileLoaded: boolean
    startFileFromPackage: boolean
    entryAnalyzed: boolean
    canDispatchEvents: boolean
    state: string
    runtimeKind: string
    startFile: string
    entryFormat: string
    packageArgument: string
    reason?: string
    blockedReason?: string
    vmState: string
    scriptVmState: string
    scriptVmBlockedReason: string
    lastEvent: string
    startFileBytes: number
    entryChecksum: number
    entryLineCount: number
    loadedModuleCount: number
    loadedModuleBytes: number
    loadedModuleChecksum: number
    scriptGlobalCount: number
    scriptApiFunctionCount: number
    scriptImplementedApiFunctionCount: number
    scriptLegacyMappedApiCount: number
    scriptPrototypeCount: number
    scriptInstructionCount: number
    scriptDecodedInstructionCount: number
    scriptUnknownOpcodeCount: number
    scriptHasReturnOpcode: number
    scriptConstantCount: number
    scriptStringConstantCount: number
    scriptChildPrototypeCount: number
    scriptMaxStackSize: number
    scriptStackFrameDepth: number
    scriptStackSlotCount: number
    scriptGlobalReadCount: number
    scriptGlobalWriteCount: number
    scriptRegisterCount: number
    scriptRegisterWriteCount: number
    scriptExecutedInstructionCount: number
    scriptSupportedInstructionCount: number
    scriptUnsupportedInstructionCount: number
    scriptLoadConstantCount: number
    scriptMoveCount: number
    scriptGlobalOpcodeCount: number
    scriptJumpCount: number
    scriptCompareCount: number
    scriptTestCount: number
    scriptBranchSkipCount: number
    scriptArithmeticCount: number
    scriptUnaryCount: number
    scriptConcatCount: number
    scriptTableCount: number
    scriptCallCount: number
    scriptApiCallCount: number
    scriptApiCallFailureCount: number
    scriptClosureCount: number
    scriptUpvalueCount: number
    scriptLoopCount: number
    scriptVarargCount: number
    scriptClosureCallCount: number
    scriptFrameEnterCount: number
    scriptMaxCallDepth: number
    scriptUpvalueBindCount: number
    scriptOpenArgCallCount: number
    scriptOpenReturnCount: number
    scriptIteratorCallCount: number
    scriptGenericLoopCount: number
    scriptEnvironmentAccessCount: number
    scriptStandardLibraryCallCount: number
    scriptProtectedCallCount: number
    scriptRegisteredModuleCount: number
    scriptModuleLoadCount: number
    scriptModuleCacheHitCount: number
    scriptModuleFsLoadCount: number
    scriptReturnReached: number
    scriptUnsupportedOpcodeName: string
    scriptBlockedEvents: number
    queuedEvents: number
    dispatchedEvents: number
    startCount: number
    pauseCount: number
    resumeCount: number
  }
  lifecycleProbe?: {
    pause?: { ok: boolean; errorCode?: number; errorMessage?: string }
    resume?: { ok: boolean; errorCode?: number; errorMessage?: string }
  }
  lookFor?: boolean
  read?: { ok: boolean; bytes?: number; fromPackage?: boolean; errorMessage?: string }
  list?: { ok: boolean; entries?: Array<{ name: string; directory: boolean; size: number; fromPackage: boolean }> }
}

interface NativeSmokeModule {
  probePureMrpBackend(options: NativePureBackendProbeOptions): NativePureBackendProbeResult
}

const smokeNative = nativeMrp as NativeSmokeModule
const SMOKE_RAW_PACKAGE = 'mrp/mythroad/dsm_gm.mrp'
const SMOKE_PACKAGE_NAME = 'dsm_gm.mrp'

function errorToString(error: Object): string {
  if (error instanceof Error) {
    return error.message
  }
  return JSON.stringify(error)
}

function ensureDir(path: string): void {
  try {
    fs.mkdirSync(path, true)
  } catch (_error) {
  }
}

function writePackageIfNeeded(path: string, content: Uint8Array): void {
  try {
    const stat = fs.statSync(path)
    if (stat.size === content.byteLength) {
      return
    }
  } catch (_error) {
  }
  const file = fs.openSync(path, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC)
  try {
    const buffer = content.buffer.slice(content.byteOffset, content.byteOffset + content.byteLength)
    fs.writeSync(file.fd, buffer, { offset: 0, length: content.byteLength })
  } finally {
    fs.closeSync(file)
  }
}

function preparePureSmokePackage(): string | null {
  ensureRuntimeAssets()
  const content = readBundledRawFile(SMOKE_RAW_PACKAGE)
  if (!content) {
    return null
  }
  const dir = `${runtimeWorkDir()}/.pure_mrp_smoke`
  ensureDir(dir)
  const packagePath = `${dir}/${SMOKE_PACKAGE_NAME}`
  writePackageIfNeeded(packagePath, content)
  return packagePath
}

export function runPureMrpBackendSmokeTest(): boolean {
  try {
    const packagePath = preparePureSmokePackage()
    if (!packagePath) {
      console.warn(`MRP pure smoke: missing ${SMOKE_RAW_PACKAGE}`)
      return false
    }
    const result = smokeNative.probePureMrpBackend({
      workDir: runtimeWorkDir(),
      packagePath,
      path: 'start.mr',
      listDir: '',
      width: 240,
      height: 320,
      debug: true
    })
    const mappedApiCount = result.apiTable?.filter((item) => item.legacyIndex >= 0).length ?? 0
    console.info(
      `MRP pure smoke: ok=${result.ok}` +
        ` init=${result.init?.ok}` +
        ` load=${result.load?.ok}` +
        ` start=${result.start?.ok}` +
        ` app=${result.summary?.appName ?? result.summary?.internalName ?? ''}` +
        ` entries=${result.summary?.entryCount ?? 0}` +
        ` frame=${result.summary?.frameId ?? 0}` +
        ` draw=${result.summary?.drawCount ?? 0}` +
        ` size=${result.summary?.width ?? 0}x${result.summary?.height ?? 0}` +
        ` api=${result.summary?.implementedApiFunctionCount ?? 0}/${result.summary?.apiFunctionCount ?? 0}` +
        ` apiMapped=${mappedApiCount}` +
        ` timerProbe=${result.apiProbe?.timerStart?.ok}/${result.apiProbe?.getTime?.ok}/${result.apiProbe?.timerStop?.ok}` +
        ` memory=${result.summary?.memoryUsed ?? 0}/${result.summary?.memoryCapacity ?? 0}` +
        ` memoryProbe=${result.memoryProbe?.ok}` +
        ` readFileProbe=${result.readFileProbe?.ok}` +
        ` readFileBytes=${result.readFileProbe?.bytes ?? 0}` +
        ` fileHandleProbe=${result.fileHandleProbe?.ok}` +
        ` fileWrite=${result.fileHandleProbe?.writeBytes ?? 0}` +
        ` fileRead=${result.fileHandleProbe?.readBytes ?? 0}` +
        ` fileLen=${result.fileHandleProbe?.getLenResult ?? -1}` +
        ` infoProbe=${result.infoProbe?.ok}` +
        ` info=${result.infoProbe?.fileInfoResult ?? 0}/${result.infoProbe?.dirInfoResult ?? 0}/${result.infoProbe?.missingInfoResult ?? 0}` +
        ` screenInfoProbe=${result.screenInfoProbe?.ok}` +
        ` screen=${result.screenInfoProbe?.width ?? 0}x${result.screenInfoProbe?.height ?? 0}x${result.screenInfoProbe?.bit ?? 0}` +
        ` userInfoProbe=${result.userInfoProbe?.ok}` +
        ` user=${result.userInfoProbe?.manufactory ?? ''}/${result.userInfoProbe?.type ?? ''}/${result.userInfoProbe?.version ?? 0}` +
        ` datetimeProbe=${result.datetimeProbe?.ok}` +
        ` datetime=${result.datetimeProbe?.year ?? 0}-${result.datetimeProbe?.month ?? 0}-${result.datetimeProbe?.day ?? 0}` +
        ` legacyDispatchProbe=${result.legacyDispatchProbe?.ok}` +
        ` legacyFrame=${result.legacyDispatchProbe?.frameId ?? 0}` +
        ` directoryProbe=${result.directoryProbe?.ok}` +
        ` dirFirst=${result.directoryProbe?.firstEntry ?? ''}` +
        ` dirNext=${result.directoryProbe?.nextEntry ?? ''}` +
        ` startup=${result.startupProbe?.runtimeKind ?? ''}/${result.startupProbe?.startFile ?? ''}` +
        ` bridge=${result.startupProbe?.usesCFunctionBridge}` +
        ` executor=${result.executorProbe?.state ?? ''}/${result.executorProbe?.runnable}` +
        ` startFileLoaded=${result.executorProbe?.startFileLoaded}/${result.executorProbe?.startFileBytes ?? 0}` +
        ` entry=${result.executorProbe?.entryFormat ?? ''}/${result.executorProbe?.entryChecksum ?? 0}` +
        ` modules=${result.executorProbe?.loadedModuleCount ?? 0}/${result.executorProbe?.loadedModuleBytes ?? 0}` +
        ` events=${result.executorProbe?.dispatchedEvents ?? 0}/${result.executorProbe?.lastEvent ?? ''}` +
        ` scriptVm=${result.executorProbe?.scriptVmState ?? ''}/${result.executorProbe?.scriptGlobalCount ?? 0}` +
        ` scriptApi=${result.executorProbe?.scriptImplementedApiFunctionCount ?? 0}/${result.executorProbe?.scriptApiFunctionCount ?? 0}` +
        ` scriptProto=${result.executorProbe?.scriptPrototypeCount ?? 0}/${result.executorProbe?.scriptInstructionCount ?? 0}` +
        ` scriptOpcode=${result.executorProbe?.scriptDecodedInstructionCount ?? 0}/${result.executorProbe?.scriptUnknownOpcodeCount ?? 0}/${result.executorProbe?.scriptHasReturnOpcode ?? 0}` +
        ` scriptConst=${result.executorProbe?.scriptConstantCount ?? 0}/${result.executorProbe?.scriptStringConstantCount ?? 0}` +
        ` scriptStack=${result.executorProbe?.scriptStackFrameDepth ?? 0}/${result.executorProbe?.scriptStackSlotCount ?? 0}` +
        ` scriptGlobal=${result.executorProbe?.scriptGlobalReadCount ?? 0}/${result.executorProbe?.scriptGlobalWriteCount ?? 0}` +
        ` scriptExec=${result.executorProbe?.scriptExecutedInstructionCount ?? 0}/${result.executorProbe?.scriptSupportedInstructionCount ?? 0}/${result.executorProbe?.scriptUnsupportedInstructionCount ?? 0}` +
        ` scriptRegs=${result.executorProbe?.scriptRegisterCount ?? 0}/${result.executorProbe?.scriptRegisterWriteCount ?? 0}` +
        ` scriptOps=${result.executorProbe?.scriptLoadConstantCount ?? 0}/${result.executorProbe?.scriptMoveCount ?? 0}/${result.executorProbe?.scriptGlobalOpcodeCount ?? 0}/${result.executorProbe?.scriptReturnReached ?? 0}` +
        ` scriptFlow=${result.executorProbe?.scriptJumpCount ?? 0}/${result.executorProbe?.scriptCompareCount ?? 0}/${result.executorProbe?.scriptTestCount ?? 0}/${result.executorProbe?.scriptBranchSkipCount ?? 0}` +
        ` scriptData=${result.executorProbe?.scriptArithmeticCount ?? 0}/${result.executorProbe?.scriptUnaryCount ?? 0}/${result.executorProbe?.scriptConcatCount ?? 0}/${result.executorProbe?.scriptTableCount ?? 0}/${result.executorProbe?.scriptCallCount ?? 0}` +
        ` scriptApiCall=${result.executorProbe?.scriptApiCallCount ?? 0}/${result.executorProbe?.scriptApiCallFailureCount ?? 0}` +
        ` scriptFunc=${result.executorProbe?.scriptClosureCount ?? 0}/${result.executorProbe?.scriptUpvalueCount ?? 0}/${result.executorProbe?.scriptLoopCount ?? 0}/${result.executorProbe?.scriptVarargCount ?? 0}` +
        ` scriptFrames=${result.executorProbe?.scriptClosureCallCount ?? 0}/${result.executorProbe?.scriptFrameEnterCount ?? 0}/${result.executorProbe?.scriptMaxCallDepth ?? 0}/${result.executorProbe?.scriptUpvalueBindCount ?? 0}` +
        ` scriptOpen=${result.executorProbe?.scriptOpenArgCallCount ?? 0}/${result.executorProbe?.scriptOpenReturnCount ?? 0}` +
        ` scriptIter=${result.executorProbe?.scriptIteratorCallCount ?? 0}/${result.executorProbe?.scriptGenericLoopCount ?? 0}` +
        ` scriptEnv=${result.executorProbe?.scriptEnvironmentAccessCount ?? 0}` +
        ` scriptStd=${result.executorProbe?.scriptStandardLibraryCallCount ?? 0}` +
        ` scriptPcall=${result.executorProbe?.scriptProtectedCallCount ?? 0}` +
        ` scriptModule=${result.executorProbe?.scriptRegisteredModuleCount ?? 0}/${result.executorProbe?.scriptModuleLoadCount ?? 0}/${result.executorProbe?.scriptModuleCacheHitCount ?? 0}/${result.executorProbe?.scriptModuleFsLoadCount ?? 0}` +
        ` scriptStop=${result.executorProbe?.scriptUnsupportedOpcodeName ?? ''}` +
        ` scriptBlockedEvents=${result.executorProbe?.scriptBlockedEvents ?? 0}` +
        ` lifecycle=${result.lifecycleProbe?.pause?.ok}/${result.lifecycleProbe?.resume?.ok}` +
        ` wait=${result.executorProbe?.waitingForScriptVm}/${result.executorProbe?.waitingForCFunctionBridge}` +
        ` blocked=${result.executorProbe?.blockedReason ?? ''}` +
        ` mrc=${result.startupProbe?.mrcModules?.length ?? 0}` +
        ` assets=${(result.startupProbe?.images?.length ?? 0) + (result.startupProbe?.audio?.length ?? 0)}` +
        ` lookFor=${result.lookFor}` +
        ` readBytes=${result.read?.bytes ?? 0}` +
        ` listEntries=${result.list?.entries?.length ?? 0}`
    )
    return result.ok
  } catch (error) {
    console.error(`MRP pure smoke failed: ${errorToString(error as Object)}`)
    return false
  }
}
