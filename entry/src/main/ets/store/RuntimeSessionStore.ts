import { RuntimeService } from '../services/RuntimeService'
import type { UiError } from '../models/UiError'
import type { RuntimePhase } from '../models/RuntimeSessionState'

export class RuntimeSessionStore {
  currentAppId: string | null = null
  phase: RuntimePhase = 'idle'
  error: UiError | null = null
  canRestart: boolean = false
  canExit: boolean = false

  constructor(private readonly service: RuntimeService = new RuntimeService()) {}

  async startSession(appId: string): Promise<void> {
    this.currentAppId = appId
    this.phase = 'starting'
    this.error = null
    try {
      await this.service.start(appId)
      this.phase = 'running'
      this.canRestart = true
      this.canExit = true
    } catch (_error) {
      this.phase = 'error'
      this.error = { message: '启动失败', retryable: true }
    }
  }

  async restartSession(): Promise<void> {
    if (!this.currentAppId) {
      return
    }
    try {
      await this.service.restart(this.currentAppId)
      this.phase = 'running'
    } catch (_error) {
      this.phase = 'error'
      this.error = { message: '重启失败', retryable: true }
    }
  }

  async stopSession(): Promise<void> {
    await this.service.stop()
    this.phase = 'stopped'
    this.canExit = false
    this.canRestart = false
  }

  clearError(): void {
    this.error = null
  }
}
