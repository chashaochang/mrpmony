import { RuntimeSessionStore } from '../store/RuntimeSessionStore'
import { PageHeader } from '../components/common/PageHeader'
import { StatusBanner } from '../components/common/StatusBanner'
import { ConfirmDialog } from '../components/common/ConfirmDialog'
import { RuntimeContainer } from '../components/runtime/RuntimeContainer'
import { RuntimeToolbar } from '../components/runtime/RuntimeToolbar'
import { RuntimeStatusOverlay } from '../components/runtime/RuntimeStatusOverlay'

@Component
export struct RunPage {
  private store: RuntimeSessionStore = new RuntimeSessionStore()

  private onBack(): void {
    console.info('TODO: back to AppListPage')
  }

  private async onRestart(): Promise<void> {
    await this.store.restartSession()
  }

  private async onExit(): Promise<void> {
    await this.store.stopSession()
  }

  build() {
    Column() {
      PageHeader({ title: '运行页' })
      if (this.store.error) {
        StatusBanner({ text: this.store.error.message })
      }
      RuntimeToolbar({
        onBack: () => this.onBack(),
        onRestart: () => this.onRestart(),
        onExit: () => this.onExit()
      })
      RuntimeContainer()
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
