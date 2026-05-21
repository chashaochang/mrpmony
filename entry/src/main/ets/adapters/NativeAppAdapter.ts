import fs from '@ohos.file.fs'
import { util } from '@kit.ArkTS'
import { ensureRuntimeAssets, runtimeMythroadDir } from './RuntimeAssetBootstrap'

export interface DeleteInstalledAppOptions {
  deleteRelatedData?: boolean
}

export interface NativeInstalledAppDTO {
  appId: string
  name: string
  fileName: string
  description?: string
  icon?: string
  version?: string
  addedAt?: number
  sizeBytes?: number
  runnable: boolean
}

interface MrpHeaderMeta {
  appName?: string
  description?: string
  version?: string
}

function errorToString(error: Object): string {
  if (error instanceof Error) {
    return error.message
  }
  return JSON.stringify(error)
}

export class NativeAppAdapter {
  private static readonly MRP_HEADER_SIZE: number = 240
  private static readonly MRP_APP_NAME_OFFSET: number = 28
  private static readonly MRP_APP_NAME_SIZE: number = 24
  private static readonly MRP_DESCRIPTION_OFFSET: number = 128
  private static readonly MRP_DESCRIPTION_SIZE: number = 64
  private static readonly MRP_VERSION_OFFSET: number = 72
  private static readonly HIDDEN_RUNTIME_MRPS: string[] = [
    'dsm_gm.mrp',
    'ydqtwo.mrp',
    'mpc.mrp',
    'flaengine.mrp'
  ]
  private static readonly mrpTextDecoder: util.TextDecoder = util.TextDecoder.create('gbk', {
    fatal: false,
    ignoreBOM: true
  })

  async listInstalledApps(): Promise<NativeInstalledAppDTO[]> {
    ensureRuntimeAssets()
    const mythroadDir = runtimeMythroadDir()
    let files: string[] = []
    try {
      if (!fs.accessSync(mythroadDir)) {
        console.info(`MRP list: mythroad dir missing: ${mythroadDir}`)
        return []
      }
      files = fs.listFileSync(mythroadDir)
    } catch (_error) {
      console.info(`MRP list: mythroad dir unavailable: ${mythroadDir}`)
      return []
    }
    console.info(`MRP list: ${mythroadDir} => ${files.join(',')}`)
    const appFiles = files
      .filter((name: string) => name.toLowerCase().endsWith('.mrp'))
      .filter((name: string) => !this.isHiddenRuntimeMrp(name))
      .sort((a: string, b: string) => a.localeCompare(b))
    return this.buildInstalledAppsAsync(mythroadDir, appFiles)
  }

  async deleteInstalledApp(appId: string, options?: DeleteInstalledAppOptions): Promise<void> {
    ensureRuntimeAssets()
    if (!this.isSafeMrpFileName(appId)) {
      throw new Error(`Invalid MRP app id: ${appId}`)
    }
    const mythroadDir = runtimeMythroadDir()
    const targetPath = `${mythroadDir}/${appId}`
    let appDeleted = false
    try {
      if (!fs.accessSync(targetPath)) {
        console.info(`MRP delete: already missing ${targetPath}`)
        appDeleted = true
      }
    } catch (_error) {
      console.info(`MRP delete: already missing ${targetPath}`)
      appDeleted = true
    }
    if (!appDeleted) {
      try {
        fs.unlinkSync(targetPath)
        appDeleted = true
        console.info(`MRP delete: unlink ${targetPath}`)
      } catch (unlinkError) {
        const fallbackPath = `${targetPath}.deleted-${Date.now()}`
        try {
          fs.renameSync(targetPath, fallbackPath)
          appDeleted = true
          console.info(`MRP delete: renamed ${targetPath} -> ${fallbackPath}`)
        } catch (renameError) {
          const unlinkMessage = errorToString(unlinkError as Object)
          const renameMessage = errorToString(renameError as Object)
          console.error(`MRP delete failed: ${targetPath}: unlink=${unlinkMessage}; rename=${renameMessage}`)
          throw new Error(`delete failed: ${unlinkMessage}`)
        }
      }
    }
    if (options?.deleteRelatedData === true) {
      this.deleteRelatedData(appId)
    }
  }

  private deleteRelatedData(appId: string): void {
    const mythroadDir = runtimeMythroadDir()
    const baseName = this.displayName(appId).toLowerCase()
    const appName = appId.toLowerCase()
    const diskDirs = [
      `${mythroadDir}/disk/a`,
      `${mythroadDir}/disk/b`,
      `${mythroadDir}/disk/x`
    ]
    for (const dir of diskDirs) {
      this.deleteMatchingChildren(dir, baseName, appName)
    }
  }

  private deleteMatchingChildren(dir: string, baseName: string, appName: string): void {
    let children: string[] = []
    try {
      if (!fs.accessSync(dir)) {
        return
      }
      children = fs.listFileSync(dir)
    } catch (_error) {
      return
    }
    for (const child of children) {
      if (!this.isRelatedDataName(child, baseName, appName)) {
        continue
      }
      this.deletePathRecursive(`${dir}/${child}`)
    }
  }

  private isRelatedDataName(name: string, baseName: string, appName: string): boolean {
    const lower = name.toLowerCase()
    if (lower === baseName || lower === appName) {
      return true
    }
    return lower.startsWith(`${baseName}.`) ||
      lower.startsWith(`${baseName}_`) ||
      lower.startsWith(`${baseName}-`) ||
      lower.startsWith(`${appName}.`) ||
      lower.startsWith(`${appName}_`) ||
      lower.startsWith(`${appName}-`)
  }

  private deletePathRecursive(path: string): void {
    try {
      const stat = fs.statSync(path)
      if (stat.isDirectory()) {
        const children = fs.listFileSync(path)
        for (const child of children) {
          this.deletePathRecursive(`${path}/${child}`)
        }
        fs.rmdirSync(path)
      } else {
        fs.unlinkSync(path)
      }
      console.info(`MRP delete related data: ${path}`)
    } catch (error) {
      console.warn(`MRP delete related data failed: ${path}: ${errorToString(error as Object)}`)
    }
  }

  private isSafeMrpFileName(fileName: string): boolean {
    if (fileName.length === 0) {
      return false
    }
    if (!fileName.toLowerCase().endsWith('.mrp')) {
      return false
    }
    if (fileName.includes('/') || fileName.includes('\\') || fileName.includes('..')) {
      return false
    }
    return true
  }

  private isHiddenRuntimeMrp(fileName: string): boolean {
    const lower = fileName.toLowerCase()
    return NativeAppAdapter.HIDDEN_RUNTIME_MRPS.includes(lower)
  }

  private fileStat(filePath: string): fs.Stat | undefined {
    try {
      return fs.statSync(filePath)
    } catch (_error) {
      return undefined
    }
  }

  private displayName(fileName: string): string {
    const suffixIndex = fileName.toLowerCase().lastIndexOf('.mrp')
    return suffixIndex > 0 ? fileName.substring(0, suffixIndex) : fileName
  }

  private readMrpHeaderMeta(filePath: string): MrpHeaderMeta | undefined {
    let file: fs.File | undefined = undefined
    try {
      file = fs.openSync(filePath, fs.OpenMode.READ_ONLY)
      const buffer = new ArrayBuffer(NativeAppAdapter.MRP_HEADER_SIZE)
      const readLength = fs.readSync(file.fd, buffer, {
        offset: 0,
        length: NativeAppAdapter.MRP_HEADER_SIZE
      })
      if (readLength < NativeAppAdapter.MRP_HEADER_SIZE) {
        return undefined
      }
      const bytes = new Uint8Array(buffer)
      if (!this.hasMrpMagic(bytes)) {
        return undefined
      }
      const appName = this.decodeMrpField(bytes, NativeAppAdapter.MRP_APP_NAME_OFFSET, NativeAppAdapter.MRP_APP_NAME_SIZE)
      const description = this.decodeMrpField(bytes, NativeAppAdapter.MRP_DESCRIPTION_OFFSET, NativeAppAdapter.MRP_DESCRIPTION_SIZE)
      const version = this.readLe32(bytes, NativeAppAdapter.MRP_VERSION_OFFSET).toString()
      return {
        appName: appName.length > 0 ? appName : undefined,
        description: description.length > 0 ? description : undefined,
        version
      }
    } catch (error) {
      console.warn(`MRP header parse failed: ${filePath}: ${errorToString(error as Object)}`)
      return undefined
    } finally {
      if (file) {
        try {
          fs.closeSync(file)
        } catch (_error) {
        }
      }
    }
  }

  private hasMrpMagic(bytes: Uint8Array): boolean {
    return bytes[0] === 0x4D && bytes[1] === 0x52 && bytes[2] === 0x50 && bytes[3] === 0x47
  }

  private decodeMrpField(bytes: Uint8Array, offset: number, length: number): string {
    const end = Math.min(bytes.length, offset + length)
    let realEnd = offset
    while (realEnd < end && bytes[realEnd] !== 0) {
      realEnd += 1
    }
    while (realEnd > offset && bytes[realEnd - 1] <= 0x20) {
      realEnd -= 1
    }
    if (realEnd <= offset) {
      return ''
    }
    const field = new Uint8Array(bytes.buffer.slice(offset, realEnd))
    return NativeAppAdapter.mrpTextDecoder.decodeToString(field, { stream: false }).trim()
  }

  private readLe32(bytes: Uint8Array, offset: number): number {
    return (bytes[offset] |
      (bytes[offset + 1] << 8) |
      (bytes[offset + 2] << 16) |
      (bytes[offset + 3] << 24)) >>> 0
  }

  private buildInstalledAppsAsync(mythroadDir: string, appFiles: string[]): Promise<NativeInstalledAppDTO[]> {
    return new Promise<NativeInstalledAppDTO[]>((resolve: (value: NativeInstalledAppDTO[]) => void) => {
      const result: NativeInstalledAppDTO[] = []
      let index = 0
      const batchSize = 24
      const processBatch = (): void => {
        const end = Math.min(index + batchSize, appFiles.length)
        while (index < end) {
          const fileName = appFiles[index]
          const filePath = `${mythroadDir}/${fileName}`
          const fileStat = this.fileStat(filePath)
          const metadata = this.readMrpHeaderMeta(filePath)
          result.push({
            appId: fileName,
            name: metadata?.appName || this.displayName(fileName),
            fileName,
            description: metadata?.description,
            icon: undefined,
            version: metadata?.version,
            addedAt: fileStat?.mtime,
            sizeBytes: fileStat?.size,
            runnable: true
          })
          index += 1
        }
        if (index >= appFiles.length) {
          resolve(result)
          return
        }
        setTimeout((): void => {
          processBatch()
        }, 0)
      }
      processBatch()
    })
  }
}
