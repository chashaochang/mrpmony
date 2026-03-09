export interface NativeInstalledAppDTO {
  appId: string
  name: string
  icon?: string
  version?: string
  runnable: boolean
}

export class NativeAppAdapter {
  async listInstalledApps(): Promise<NativeInstalledAppDTO[]> {
    return [
      {
        appId: 'demo.app.001',
        name: 'Demo MRP App',
        version: '1.0.0',
        runnable: true
      },
      {
        appId: 'demo.app.002',
        name: 'Broken Sample',
        version: '0.9.1',
        runnable: false
      }
    ]
  }
}
