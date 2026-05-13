import common from '@ohos.app.ability.common'
import { ImportService } from '../services/ImportService'
import type { PackagePreviewVM } from '../models/PackagePreviewVM'
import type { UiError } from '../models/UiError'
import { StoreObserver } from './StoreObserver'

export type ImportPhase = 'idle' | 'picking' | 'analyzing' | 'importing' | 'success' | 'failed'

export class ImportStore extends StoreObserver {
  phase: ImportPhase = 'idle'
  selectedFile: string | null = null
  selectedFiles: string[] = []
  previewInfo: PackagePreviewVM | null = null
  previewInfos: PackagePreviewVM[] = []
  importedCount: number = 0
  debugStatus: string = '等待选择文件'
  error: UiError | null = null

  constructor(private readonly service: ImportService = new ImportService()) {
    super()
  }

  configureContext(context: common.UIAbilityContext): void {
    this.service.configureContext(context)
  }

  async pickAndAnalyze(): Promise<void> {
    await this.pickFile()
    await this.analyzeSelectedFile()
  }

  async pickFile(): Promise<void> {
    this.phase = 'picking'
    this.error = null
    this.debugStatus = '正在打开系统文件选择器'
    this.notify()
    try {
      this.selectedFiles = await this.service.pickMrpFiles()
      this.debugStatus = `文件选择器返回 ${this.selectedFiles.length} 个文件`
      console.info(`MRP import store: ${this.debugStatus}`)
      this.selectedFile = this.selectedFiles.length > 0 ? this.selectedFiles[0] : null
      this.phase = this.selectedFiles.length > 0 ? 'analyzing' : 'idle'
      if (this.selectedFiles.length === 0) {
        this.error = { message: '没有选择文件，或文件选择器未返回可读文件', retryable: true }
      }
    } catch (error) {
      this.phase = 'failed'
      this.error = { message: `选择文件失败：${this.errorMessage(error)}`, retryable: true }
      this.debugStatus = this.error.message
    }
    this.notify()
  }

  async analyzeSelectedFile(): Promise<void> {
    if (this.selectedFiles.length === 0) {
      return
    }
    try {
      const previews: PackagePreviewVM[] = []
      for (const file of this.selectedFiles) {
        this.debugStatus = `正在检查：${file}`
        this.notify()
        previews.push(await this.service.analyzePackage(file))
      }
      this.previewInfos = previews
      this.previewInfo = this.previewInfos.length > 0 ? this.previewInfos[0] : null
      if (this.validCount() === 0) {
        this.phase = 'failed'
        this.error = { message: '请选择有效的 .mrp 文件', retryable: true }
        this.notify()
        return
      }
      if (this.validCount() < this.previewInfos.length) {
        this.error = { message: `已忽略 ${this.previewInfos.length - this.validCount()} 个无效文件`, retryable: true }
      }
      this.debugStatus = `检查完成，可导入 ${this.validCount()} 个`
      console.info(`MRP import store: ${this.debugStatus}`)
      this.phase = 'idle'
    } catch (_error) {
      this.phase = 'failed'
      this.error = { message: `包解析失败：${this.errorMessage(_error)}`, retryable: true }
      this.debugStatus = this.error.message
    }
    this.notify()
  }

  async startImport(): Promise<void> {
    if (this.selectedFiles.length === 0 || this.validCount() === 0) {
      return
    }
    this.phase = 'importing'
    this.error = null
    this.debugStatus = '正在复制文件到应用目录'
    this.notify()
    try {
      const validFiles: string[] = []
      this.previewInfos.forEach((preview: PackagePreviewVM, index: number) => {
        if (preview.valid && index < this.selectedFiles.length) {
          validFiles.push(this.selectedFiles[index])
        }
      })
      const result = await this.service.importPackages(validFiles)
      this.importedCount = result.appIds.length
      this.debugStatus = `导入完成：${this.importedCount} 个`
      console.info(`MRP import store: ${this.debugStatus}`)
      this.phase = 'success'
    } catch (_error) {
      this.phase = 'failed'
      this.error = { message: `导入失败：${this.errorMessage(_error)}`, retryable: true }
      this.debugStatus = this.error.message
    }
    this.notify()
  }

  reset(): void {
    this.phase = 'idle'
    this.selectedFile = null
    this.selectedFiles = []
    this.previewInfo = null
    this.previewInfos = []
    this.importedCount = 0
    this.debugStatus = '等待选择文件'
    this.error = null
    this.notify()
  }

  validCount(): number {
    return this.previewInfos.filter((item: PackagePreviewVM) => item.valid).length
  }

  private errorMessage(error: Object | unknown): string {
    if (error instanceof Error) {
      return error.message
    }
    if (typeof error === 'object' && error !== null) {
      const err = error as Record<string, Object>
      const code = err['code'] ? `${err['code']}` : ''
      const message = err['message'] ? `${err['message']}` : ''
      if (code || message) {
        return `${code} ${message}`.trim()
      }
    }
    return JSON.stringify(error)
  }
}
