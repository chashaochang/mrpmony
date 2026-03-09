export interface NativePackagePreviewDTO {
  name: string
  version?: string
  icon?: string
  size?: number
  valid: boolean
}

export class NativeImportAdapter {
  async pickFile(): Promise<string | null> {
    return null
  }

  async inspectPackage(_fileUri: string): Promise<NativePackagePreviewDTO> {
    return { name: '', valid: false }
  }

  async importPackage(_fileUri: string): Promise<{ appId: string }> {
    return { appId: '' }
  }
}
