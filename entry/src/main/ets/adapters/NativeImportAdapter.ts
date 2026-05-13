import common from '@ohos.app.ability.common'
import fs from '@ohos.file.fs'
import picker from '@ohos.file.picker'
import { ensureRuntimeAssets, runtimeMythroadDir } from './RuntimeAssetBootstrap'

export interface NativePackagePreviewDTO {
  name: string
  version?: string
  icon?: string
  size?: number
  valid: boolean
}

export class NativeImportAdapter {
  private selectedName: string = ''
  private selectedNameByUri: Map<string, string> = new Map()
  private context?: common.UIAbilityContext

  configureContext(context: common.UIAbilityContext): void {
    this.context = context
  }

  async pickFile(): Promise<string | null> {
    const uris = await this.pickFiles()
    if (uris.length === 0) {
      return null
    }
    return uris[0]
  }

  async pickFiles(): Promise<string[]> {
    const documentPicker = this.context ? new picker.DocumentViewPicker(this.context) : new picker.DocumentViewPicker()
    const options = new picker.DocumentSelectOptions()
    options.maxSelectNumber = 1000
    console.info('MRP import: opening DocumentViewPicker')
    const uris = await documentPicker.select(options)
    console.info(`MRP import: DocumentViewPicker returned ${uris ? uris.length : 0} uri(s)`)
    if (!uris || uris.length === 0) {
      return []
    }
    this.selectedNameByUri.clear()
    uris.forEach((uri: string) => {
      this.selectedNameByUri.set(uri, this.fileNameFromUri(uri))
    })
    this.selectedName = this.selectedNameByUri.get(uris[0]) ?? ''
    return uris
  }

  async inspectPackage(fileUri: string): Promise<NativePackagePreviewDTO> {
    const name = this.safeFileName(this.selectedNameByUri.get(fileUri) || this.selectedName || this.fileNameFromUri(fileUri))
    if (!name.toLowerCase().endsWith('.mrp')) {
      return {
        name,
        valid: false
      }
    }
    return {
      name,
      valid: true
    }
  }

  async importPackage(fileUri: string): Promise<{ appId: string }> {
    ensureRuntimeAssets()
    const name = this.safeFileName(this.selectedNameByUri.get(fileUri) || this.selectedName || this.fileNameFromUri(fileUri))
    if (!name.toLowerCase().endsWith('.mrp')) {
      throw new Error('请选择 .mrp 文件')
    }
    const targetPath = `${runtimeMythroadDir()}/${name}`
    await this.copyFileAsync(fileUri, targetPath)
    const stat = fs.statSync(targetPath)
    console.info(`MRP import: copied ${name} to ${targetPath}, size=${stat.size}`)
    return {
      appId: name
    }
  }

  async importPackages(fileUris: string[]): Promise<{ appIds: string[] }> {
    const appIds: string[] = []
    for (const uri of fileUris) {
      const result = await this.importPackage(uri)
      appIds.push(result.appId)
    }
    return { appIds }
  }

  private async copyFileAsync(sourceUri: string, targetPath: string): Promise<void> {
    const sourceFile = fs.openSync(sourceUri, fs.OpenMode.READ_ONLY)
    const targetFile = fs.openSync(targetPath, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC)
    try {
      fs.copyFileSync(sourceFile.fd, targetFile.fd)
    } finally {
      fs.closeSync(sourceFile)
      fs.closeSync(targetFile)
    }
  }

  private fileNameFromUri(uri: string): string {
    const path = uri.split('?')[0]
    const index = path.lastIndexOf('/')
    const rawName = index >= 0 ? path.substring(index + 1) : path
    const decoded = decodeURIComponent(rawName)
    return decoded.length > 0 ? decoded : `imported-${Date.now()}.mrp`
  }

  private safeFileName(name: string): string {
    const trimmed = name.trim().replace(/[\\/:*?"<>|]/g, '_')
    if (trimmed.length === 0) {
      return `imported-${Date.now()}.mrp`
    }
    return trimmed
  }
}
