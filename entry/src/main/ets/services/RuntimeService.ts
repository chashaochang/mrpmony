import { NativeRuntimeAdapter } from '../adapters/NativeRuntimeAdapter'

export interface RuntimeFrameVM {
  hasFrame: boolean
  width: number
  height: number
  frameId: number
  pixelFormat: string
}

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

  async pullFrame(): Promise<RuntimeFrameVM> {
    const frame = await this.adapter.pullFrame()
    if (!frame.ok) {
      throw new Error(frame.errorMessage || 'pullFrame failed')
    }
    return {
      hasFrame: frame.hasNewFrame,
      width: frame.width,
      height: frame.height,
      frameId: frame.frameId ?? 0,
      pixelFormat: frame.pixelFormat,
    }
  }

  async onPageShow(): Promise<void> {
    await this.adapter.notifyPageShow()
  }

  async onPageHide(): Promise<void> {
    await this.adapter.notifyPageHide()
  }
}
