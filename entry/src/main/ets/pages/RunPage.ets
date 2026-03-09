import { RuntimeSessionStore } from '../store/RuntimeSessionStore'
import { PageHeader } from '../components/common/PageHeader'
import { StatusBanner } from '../components/common/StatusBanner'
import { ConfirmDialog } from '../components/common/ConfirmDialog'
import { RuntimeContainer } from '../components/runtime/RuntimeContainer'
import { RuntimeToolbar } from '../components/runtime/RuntimeToolbar'
import { RuntimeStatusOverlay } from '../components/runtime/RuntimeStatusOverlay'

@Component
export struct RunPage {
  appId?: string
  onBack?: () => void

  @State refreshTick: number = 0
  private store: RuntimeSessionStore = new RuntimeSessionStore()
  private unsubscribe?: () => void

  aboutToAppear(): void {
    if (!this.unsubscribe) {
      this.unsubscribe = this.store.subscribe(() => {
        this.refreshTick += 1
      })
    }
    if (this.appId) {
      this.store.startSession(this.appId)
    }
  }

  aboutToDisappear(): void {
    if (this.unsubscribe) {
      this.unsubscribe()
      this.unsubscribe = undefined
    }
  }

  private onBackClick(): void {
    if (this.onBack) {
      this.onBack()
      return
    }
    console.info('TODO: back to AppListPage')
  }

  private async onRestart(): Promise<void> {
    await this.store.restartSession()
  }

  private async onExit(): Promise<void> {
    await this.store.stopSession()
    if (this.onBack) {
      this.onBack()
    }
  }

  build() {
    Column() {
      Text(`${this.refreshTick}`).fontSize(1).opacity(0)
      PageHeader({ title: '运行页' })
      if (this.appId) {
        StatusBanner({ text: `当前应用：${this.appId}` })
      }
      StatusBanner({ text: this.store.runtimeHint })
      if (this.store.error) {
        StatusBanner({ text: this.store.error.message })
      }
      RuntimeToolbar({
        canRestart: this.store.canRestart,
        canExit: this.store.canExit,
        onBack: () => this.onBackClick(),
        onRestart: () => this.onRestart(),
        onExit: () => this.onExit()
      })
      RuntimeContainer({ hint: this.store.runtimeHint })
      if (this.store.phase !== 'running') {
        RuntimeStatusOverlay({ text: `当前状态：${this.store.phase}` })
      }
      if (this.store.phase === 'stopped') {
        ConfirmDialog({ title: '运行已结束', content: '当前会话已停止' })
      }
    }
    .width('100%')
    .height('100%')
  }
}
