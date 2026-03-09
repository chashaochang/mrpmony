const sleep = (ms: number): Promise<void> => new Promise((resolve) => setTimeout(resolve, ms))

export class NativeRuntimeAdapter {
  async startSession(_appId: string): Promise<void> {
    await sleep(600)
  }

  async restartSession(_appId: string): Promise<void> {
    await sleep(400)
  }

  async stopSession(): Promise<void> {
    return
  }

  async notifyPageShow(): Promise<void> {
    return
  }

  async notifyPageHide(): Promise<void> {
    return
  }
}
