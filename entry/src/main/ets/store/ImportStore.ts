import { ImportService } from '../services/ImportService'
import type { PackagePreviewVM } from '../models/PackagePreviewVM'
import type { UiError } from '../models/UiError'

export type ImportPhase = 'idle' | 'picking' | 'analyzing' | 'importing' | 'success' | 'failed'

export class ImportStore {
  phase: ImportPhase = 'idle'
  selectedFile: string | null = null
  previewInfo: PackagePreviewVM | null = null
  error: UiError | null = null

  constructor(private readonly service: ImportService = new ImportService()) {}

  async pickFile(): Promise<void> {
    this.phase = 'picking'
    this.selectedFile = await this.service.pickMrpFile()
    this.phase = this.selectedFile ? 'analyzing' : 'idle'
  }

  async analyzeSelectedFile(): Promise<void> {
    if (!this.selectedFile) {
      return
    }
    try {
      this.previewInfo = await this.service.analyzePackage(this.selectedFile)
      this.phase = 'idle'
    } catch (_error) {
      this.phase = 'failed'
      this.error = { message: '包解析失败', retryable: true }
    }
  }

  async startImport(): Promise<void> {
    if (!this.selectedFile) {
      return
    }
    this.phase = 'importing'
    this.error = null
    try {
      await this.service.importPackage(this.selectedFile)
      this.phase = 'success'
    } catch (_error) {
      this.phase = 'failed'
      this.error = { message: '导入失败', retryable: true }
    }
  }

  reset(): void {
    this.phase = 'idle'
    this.selectedFile = null
    this.previewInfo = null
    this.error = null
  }
}
