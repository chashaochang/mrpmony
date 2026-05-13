import { NativeRuntimeAdapter } from '../adapters/NativeRuntimeAdapter'

export interface RuntimeFrameVM {
  hasFrame: boolean
  exited: boolean
  width: number
  height: number
  frameId: number
  pixelFormat: string
  bufferBytes: number
}

export interface RuntimeEditRequestVM {
  hasRequest: boolean
  requestId: number
  handle: number
  type: number
  maxSize: number
  title: string
  text: string
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
      exited: frame.exited === true,
      width: frame.width,
      height: frame.height,
      frameId: frame.frameId ?? 0,
      pixelFormat: frame.pixelFormat,
      bufferBytes: frame.bufferBytes ?? 0,
    }
  }

  async pullEditRequest(): Promise<RuntimeEditRequestVM> {
    const request = await this.adapter.pullEditRequest()
    if (!request.ok) {
      throw new Error(request.errorMessage || 'pullEditRequest failed')
    }
    return {
      hasRequest: request.hasRequest === true,
      requestId: request.requestId ?? 0,
      handle: request.handle ?? 0,
      type: request.type ?? 0,
      maxSize: request.maxSize ?? 0,
      title: request.title ?? '',
      text: request.text ?? '',
    }
  }

  async submitEditResult(requestId: number, confirmed: boolean, text: string): Promise<void> {
    await this.adapter.submitEditResult(requestId, confirmed, text)
  }

  async onPageShow(): Promise<void> {
    await this.adapter.notifyPageShow()
  }

  async onPageHide(): Promise<void> {
    await this.adapter.notifyPageHide()
  }

  async sendTouch(action: 'down' | 'move' | 'up', x: number, y: number): Promise<void> {
    await this.adapter.sendTouch(action, x, y)
  }
}
