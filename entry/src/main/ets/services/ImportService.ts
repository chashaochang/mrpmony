import common from '@ohos.app.ability.common'
import { NativeImportAdapter } from '../adapters/NativeImportAdapter'
import type { PackagePreviewVM } from '../models/PackagePreviewVM'

export class ImportService {
  constructor(private readonly adapter: NativeImportAdapter = new NativeImportAdapter()) {}

  configureContext(context: common.UIAbilityContext): void {
    this.adapter.configureContext(context)
  }

  async pickMrpFile(): Promise<string | null> {
    return this.adapter.pickFile()
  }

  async pickMrpFiles(): Promise<string[]> {
    return this.adapter.pickFiles()
  }

  async analyzePackage(fileUri: string): Promise<PackagePreviewVM> {
    const result = await this.adapter.inspectPackage(fileUri)
    return {
      name: result.name,
      version: result.version,
      icon: result.icon,
      sizeText: result.size ? this.formatSize(result.size) : undefined,
      valid: result.valid
    }
  }

  async importPackage(fileUri: string): Promise<{ appId: string }> {
    return this.adapter.importPackage(fileUri)
  }

  async importPackages(fileUris: string[]): Promise<{ appIds: string[] }> {
    return this.adapter.importPackages(fileUris)
  }

  private formatSize(size: number): string {
    if (size >= 1024 * 1024) {
      return `${(size / 1024 / 1024).toFixed(2)} MB`
    }
    if (size >= 1024) {
      return `${(size / 1024).toFixed(1)} KB`
    }
    return `${size} B`
  }
}
