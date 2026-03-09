export interface NativePackagePreviewDTO {
  name: string
  version?: string
  icon?: string
  size?: number
  valid: boolean
}

export class NativeImportAdapter {
  async pickFile(): Promise<string | null> {
    return 'file://mock/demo.mrp'
  }

  async inspectPackage(_fileUri: string): Promise<NativePackagePreviewDTO> {
    return {
      name: 'Imported Demo App',
      version: '1.0.0',
      size: 204800,
      valid: true
    }
  }

  async importPackage(_fileUri: string): Promise<{ appId: string }> {
    return { appId: 'demo.imported.001' }
  }
}
