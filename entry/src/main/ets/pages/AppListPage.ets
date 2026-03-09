import { AppCatalogStore } from '../store/AppCatalogStore'
import { PageHeader } from '../components/common/PageHeader'
import { LoadingBlock } from '../components/common/LoadingBlock'
import { EmptyState } from '../components/common/EmptyState'
import { StatusBanner } from '../components/common/StatusBanner'
import { ImportEntryButton } from '../components/appList/ImportEntryButton'
import { AppListView } from '../components/appList/AppListView'

@Component
export struct AppListPage {
  @Prop refreshToken: number = 0
  onNavigateImport?: () => void
  onNavigateRun?: (appId: string) => void

  @State refreshTick: number = 0
  @State selectedAppId: string = ''
  private lastHandledRefreshToken: number = -1
  private store: AppCatalogStore = new AppCatalogStore()
  private unsubscribe?: () => void

  aboutToAppear(): void {
    if (!this.unsubscribe) {
      this.unsubscribe = this.store.subscribe(() => {
        this.refreshTick += 1
      })
    }
    this.store.loadApps()
  }

  aboutToDisappear(): void {
    if (this.unsubscribe) {
      this.unsubscribe()
      this.unsubscribe = undefined
    }
  }

  private ensureRefreshByToken(): void {
    if (this.refreshToken === this.lastHandledRefreshToken) {
      return
    }
    this.lastHandledRefreshToken = this.refreshToken
    this.store.refreshApps()
  }

  private openImportPage(): void {
    if (this.onNavigateImport) {
      this.onNavigateImport()
      return
    }
    console.info('TODO: open ImportPage')
  }

  private openRunPage(appId: string): void {
    this.selectedAppId = appId
    if (this.onNavigateRun) {
      this.onNavigateRun(appId)
      return
    }
    console.info(`TODO: open RunPage for ${appId}`)
  }

  build() {
    this.ensureRefreshByToken()

    Column() {
      Text(`${this.refreshTick}-${this.refreshToken}`).fontSize(1).opacity(0)
      PageHeader({ title: '应用列表' })
      StatusBanner({ text: '运行入口：点击“可运行”应用卡片进入运行页；导入入口：点击下方“导入应用”。' })
      if (this.selectedAppId) {
        StatusBanner({ text: `已选择应用：${this.selectedAppId}` })
      }
      if (this.store.error) {
        StatusBanner({ text: this.store.error.message })
      }
      if (this.store.loading) {
        LoadingBlock({ text: '正在加载应用列表' })
      } else if (this.store.items.length === 0) {
        EmptyState({ text: '暂无已导入应用' })
        ImportEntryButton({ onTap: () => this.openImportPage() })
      } else {
        AppListView({
          items: this.store.items,
          onItemTap: (appId: string) => this.openRunPage(appId)
        })
        ImportEntryButton({ onTap: () => this.openImportPage() })
      }
    }
    .width('100%')
    .height('100%')
  }
}
