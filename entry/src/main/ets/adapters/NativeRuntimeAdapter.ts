export class NativeRuntimeAdapter {
  async startSession(_appId: string): Promise<void> {}
  async restartSession(_appId: string): Promise<void> {}
  async stopSession(): Promise<void> {}
  async notifyPageShow(): Promise<void> {}
  async notifyPageHide(): Promise<void> {}
}
