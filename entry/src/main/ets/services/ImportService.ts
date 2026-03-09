import { NativeImportAdapter } from '../adapters/NativeImportAdapter'
import type { PackagePreviewVM } from '../models/PackagePreviewVM'

export class ImportService {
  constructor(private readonly adapter: NativeImportAdapter = new NativeImportAdapter()) {}

  async pickMrpFile(): Promise<string | null> {
    return this.adapter.pickFile()
  }

  async analyzePackage(fileUri: string): Promise<PackagePreviewVM> {
    const result = await this.adapter.inspectPackage(fileUri)
    return {
      name: result.name,
      version: result.version,
      icon: result.icon,
      sizeText: result.size ? `${result.size}` : undefined,
      valid: result.valid
    }
  }

  async importPackage(fileUri: string): Promise<{ appId: string }> {
    return this.adapter.importPackage(fileUri)
  }
}
