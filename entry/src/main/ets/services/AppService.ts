import { NativeAppAdapter } from '../adapters/NativeAppAdapter'
import type { AppItemVM } from '../models/AppItemVM'

export class AppService {
  constructor(private readonly adapter: NativeAppAdapter = new NativeAppAdapter()) {}

  async listApps(): Promise<AppItemVM[]> {
    const apps = await this.adapter.listInstalledApps()
    return apps.map((item) => ({
      id: item.appId,
      name: item.name,
      icon: item.icon,
      version: item.version,
      status: item.runnable ? 'ready' : 'invalid'
    }))
  }
}
