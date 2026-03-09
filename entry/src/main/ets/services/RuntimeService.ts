import { NativeRuntimeAdapter } from '../adapters/NativeRuntimeAdapter'

export class RuntimeService {
  constructor(private readonly adapter: NativeRuntimeAdapter = new NativeRuntimeAdapter()) {}

  async start(appId: string): Promise<void> {
    await this.adapter.startSession(appId)
  }

  async restart(appId: string): Promise<void> {
    await this.adapter.restartSession(appId)
  }

  async stop(): Promise<void> {
    await this.adapter.stopSession()
  }

  async onPageShow(): Promise<void> {
    await this.adapter.notifyPageShow()
  }

  async onPageHide(): Promise<void> {
    await this.adapter.notifyPageHide()
  }
}
