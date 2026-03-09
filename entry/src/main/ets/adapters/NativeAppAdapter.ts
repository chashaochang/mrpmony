export interface NativeInstalledAppDTO {
  appId: string
  name: string
  icon?: string
  version?: string
  runnable: boolean
}

export class NativeAppAdapter {
  async listInstalledApps(): Promise<NativeInstalledAppDTO[]> {
    return []
  }
}
