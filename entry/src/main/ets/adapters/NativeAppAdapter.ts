import fs from '@ohos.file.fs'
import { ensureRuntimeAssets, readBundledRawFile, runtimeMythroadDir } from './RuntimeAssetBootstrap'

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

interface VmrpCatalogRecord {
  file?: string
  FileName?: string
  DisplayName?: string
  Desc?: string
}

interface VmrpCatalogMeta {
  displayName: string
  description?: string
}

function errorToString(error: Object): string {
  if (error instanceof Error) {
    return error.message
  }
  return JSON.stringify(error)
}

export class NativeAppAdapter {
  private static readonly CATALOG_PATH: string = 'mrp/catalog/data.txt'
  private static readonly HIDDEN_RUNTIME_MRPS: string[] = [
    'dsm_gm.mrp',
    'ydqtwo.mrp',
    'mpc.mrp',
    'flaengine.mrp'
  ]
  private static catalogLoaded: boolean = false
  private static catalogByKey: Map<string, VmrpCatalogMeta> = new Map()

  async listInstalledApps(): Promise<NativeInstalledAppDTO[]> {
    ensureRuntimeAssets()
    this.ensureCatalogLoaded()
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

  private lookupCatalog(fileName: string): VmrpCatalogMeta | undefined {
    const key = fileName.toLowerCase()
    const exact = NativeAppAdapter.catalogByKey.get(key)
    if (exact) {
      return exact
    }
    const baseName = this.displayName(fileName).toLowerCase()
    return NativeAppAdapter.catalogByKey.get(baseName)
  }

  private ensureCatalogLoaded(): void {
    if (NativeAppAdapter.catalogLoaded) {
      return
    }
    NativeAppAdapter.catalogLoaded = true
    const bytes = readBundledRawFile(NativeAppAdapter.CATALOG_PATH)
    if (!bytes || bytes.length === 0) {
      console.warn(`MRP catalog not found: ${NativeAppAdapter.CATALOG_PATH}`)
      return
    }

    const content = this.decodeUtf8(bytes)
    const lines = content.split('\n')
    for (const line of lines) {
      const text = line.trim()
      if (text.length === 0) {
        continue
      }
      try {
        const item = JSON.parse(text) as VmrpCatalogRecord
        const displayName = (item.DisplayName || '').trim()
        if (!displayName) {
          continue
        }
        const description = (item.Desc || '').trim()
        this.addCatalogEntry(item.file, displayName, description)
        this.addCatalogEntry(item.FileName, displayName, description)
      } catch (error) {
        // Ignore malformed lines and continue loading valid metadata.
      }
    }
    console.info(`MRP catalog loaded: ${NativeAppAdapter.catalogByKey.size} keys`)
  }

  private addCatalogEntry(rawKey: string | undefined, displayName: string, description?: string): void {
    if (!rawKey) {
      return
    }
    const normalized = rawKey.trim().toLowerCase()
    if (normalized.length === 0) {
      return
    }
    if (NativeAppAdapter.catalogByKey.has(normalized)) {
      return
    }
    NativeAppAdapter.catalogByKey.set(normalized, {
      displayName,
      description: description || undefined
    })
  }

  private decodeUtf8(bytes: Uint8Array): string {
    let output = ''
    let index = 0
    while (index < bytes.length) {
      const first = bytes[index++]
      if ((first & 0x80) === 0) {
        output += String.fromCharCode(first)
        continue
      }
      if ((first & 0xE0) === 0xC0 && index < bytes.length) {
        const second = bytes[index++]
        const code = ((first & 0x1F) << 6) | (second & 0x3F)
        output += String.fromCharCode(code)
        continue
      }
      if ((first & 0xF0) === 0xE0 && index + 1 < bytes.length) {
        const second = bytes[index++]
        const third = bytes[index++]
        const code = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F)
        output += String.fromCharCode(code)
        continue
      }
      if ((first & 0xF8) === 0xF0 && index + 2 < bytes.length) {
        const second = bytes[index++]
        const third = bytes[index++]
        const fourth = bytes[index++]
        let codePoint = ((first & 0x07) << 18) | ((second & 0x3F) << 12) | ((third & 0x3F) << 6) | (fourth & 0x3F)
        codePoint -= 0x10000
        const high = 0xD800 + (codePoint >> 10)
        const low = 0xDC00 + (codePoint & 0x3FF)
        output += String.fromCharCode(high, low)
        continue
      }
      output += '\uFFFD'
    }
    return output
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
          const metadata = this.lookupCatalog(fileName)
          const filePath = `${mythroadDir}/${fileName}`
          const fileStat = this.fileStat(filePath)
          result.push({
            appId: fileName,
            name: metadata?.displayName || this.displayName(fileName),
            fileName,
            description: metadata?.description,
            icon: undefined,
            version: undefined,
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
