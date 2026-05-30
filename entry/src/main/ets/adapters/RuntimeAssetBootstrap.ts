import common from '@ohos.app.ability.common'
import fs from '@ohos.file.fs'

let appContext: common.UIAbilityContext | null = null

interface RuntimeAsset {
  rawPath: string
  targetPath: string
}

export interface RuntimeAssetStatus {
  ok: boolean
  workDir: string
  message: string
  copiedFiles: string[]
  error?: string
}

const RAW_FILES: RuntimeAsset[] = [
  { rawPath: 'mrp/cfunction.ext', targetPath: 'cfunction.ext' },
  { rawPath: 'mrp/mythroad/dsm_gm.mrp', targetPath: 'mythroad/dsm_gm.mrp' },
  { rawPath: 'mrp/mythroad/ydqtwo.mrp', targetPath: 'mythroad/ydqtwo.mrp' },
  { rawPath: 'mrp/mythroad/mpc.mrp', targetPath: 'mythroad/mpc.mrp' },
  { rawPath: 'mrp/mythroad/plugins/flaengine.mrp', targetPath: 'mythroad/plugins/flaengine.mrp' },
  { rawPath: 'mrp/mythroad/system/gb12.uc2', targetPath: 'mythroad/system/gb12.uc2' },
  { rawPath: 'mrp/mythroad/system/gb16.uc2', targetPath: 'mythroad/system/gb16.uc2' },
]
const ASSET_MARKER = '.mrp_runtime_assets_v4.ready'
const MUTABLE_DIRS: string[] = [
  'mythroad',
  'mythroad/disk',
  'mythroad/disk/a',
  'mythroad/disk/b',
  'mythroad/disk/x',
  'mythroad/plugins',
  'mythroad/system',
]

let lastStatus: RuntimeAssetStatus = {
  ok: false,
  workDir: '',
  message: 'runtime assets not prepared',
  copiedFiles: [],
}

function parentDir(path: string): string {
  const index = path.lastIndexOf('/')
  return index > 0 ? path.substring(0, index) : path
}

function errorToString(error: Object): string {
  if (error instanceof Error) {
    return error.message
  }
  return JSON.stringify(error)
}

function ensureDir(path: string): void {
  try {
    fs.mkdirSync(path, true)
  } catch (error) {
    if (!fs.accessSync(path)) {
      throw new Error(`mkdir failed: ${path}: ${errorToString(error as Object)}`)
    }
  }
}

function writeFile(path: string, content: Uint8Array): number {
  ensureDir(parentDir(path))
  const file = fs.openSync(path, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC)
  try {
    const buffer = content.buffer.slice(content.byteOffset, content.byteOffset + content.byteLength)
    const written = fs.writeSync(file.fd, buffer, { offset: 0, length: content.byteLength })
    if (written !== content.byteLength) {
      throw new Error(`short write: ${path}: ${written}/${content.byteLength}`)
    }
    return written
  } finally {
    fs.closeSync(file)
  }
}

function hasReadyMarker(baseDir: string): boolean {
  try {
    const stat = fs.statSync(`${baseDir}/${ASSET_MARKER}`)
    return stat.size > 0
  } catch (_error) {
    return false
  }
}

function writeReadyMarker(baseDir: string): void {
  writeFile(`${baseDir}/${ASSET_MARKER}`, new Uint8Array([49]))
}

function needsWrite(path: string, expectedSize: number): boolean {
  try {
    const stat = fs.statSync(path)
    return stat.size !== expectedSize
  } catch (_error) {
    return true
  }
}

function syncBundledAssets(baseDir: string): string[] {
  if (!appContext) {
    return []
  }
  const copiedFiles: string[] = []
  for (const asset of RAW_FILES) {
    const content = appContext.resourceManager.getRawFileContentSync(asset.rawPath)
    const target = `${baseDir}/${asset.targetPath}`
    if (!needsWrite(target, content.byteLength)) {
      continue
    }
    writeFile(target, content)
    const stat = fs.statSync(target)
    if (stat.size !== content.byteLength) {
      throw new Error(`size mismatch: ${asset.targetPath}: ${stat.size}/${content.byteLength}`)
    }
    copiedFiles.push(`${asset.targetPath} (${stat.size})`)
  }
  return copiedFiles
}

export function configureRuntimeAssets(context: common.UIAbilityContext): void {
  appContext = context
  try {
    ensureRuntimeAssets()
  } catch (error) {
    lastStatus = {
      ok: false,
      workDir: runtimeWorkDir(),
      message: 'prepare runtime assets failed',
      copiedFiles: [],
      error: errorToString(error as Object),
    }
    console.error(`prepare runtime assets failed: ${lastStatus.error}`)
  }
}

export function runtimeWorkDir(): string {
  return appContext ? appContext.filesDir : '/data/storage/el2/base/files'
}

export function runtimeMythroadDir(): string {
  return `${runtimeWorkDir()}/mythroad`
}

export function runtimeIconCacheDir(): string {
  return `${runtimeWorkDir()}/mrp-icons`
}

export function runtimeAssetStatus(): RuntimeAssetStatus {
  return {
    ok: lastStatus.ok,
    workDir: lastStatus.workDir,
    message: lastStatus.message,
    copiedFiles: [...lastStatus.copiedFiles],
    error: lastStatus.error,
  }
}

export function readBundledRawFile(rawPath: string): Uint8Array | null {
  if (!appContext) {
    return null
  }
  try {
    return appContext.resourceManager.getRawFileContentSync(rawPath)
  } catch (error) {
    console.warn(`read raw file failed: ${rawPath}: ${errorToString(error as Object)}`)
    return null
  }
}

export function ensureRuntimeAssets(): string {
  if (!appContext) {
    return runtimeWorkDir()
  }
  const baseDir = runtimeWorkDir()
  for (const dir of MUTABLE_DIRS) {
    ensureDir(`${baseDir}/${dir}`)
  }
  const copiedFiles = syncBundledAssets(baseDir)
  if (hasReadyMarker(baseDir) && copiedFiles.length === 0) {
    lastStatus = {
      ok: true,
      workDir: baseDir,
      message: 'runtime assets already prepared',
      copiedFiles,
    }
    return baseDir
  }
  writeReadyMarker(baseDir)
  lastStatus = {
    ok: true,
    workDir: baseDir,
    message: copiedFiles.length > 0 ? 'runtime assets refreshed' : 'runtime assets prepared',
    copiedFiles,
  }
  return baseDir
}
