import { AppCatalogStore } from '../store/AppCatalogStore'
import { PageHeader } from '../components/common/PageHeader'
import { LoadingBlock } from '../components/common/LoadingBlock'
import { EmptyState } from '../components/common/EmptyState'
import { StatusBanner } from '../components/common/StatusBanner'
import { ImportEntryButton } from '../components/appList/ImportEntryButton'
import { AppListView } from '../components/appList/AppListView'

@Entry
@Component
struct AppListPage {
  private store: AppCatalogStore = new AppCatalogStore()

  aboutToAppear(): void {
    this.store.loadApps()
  }

  build() {
    Column() {
      PageHeader({ title: '应用列表' })
      if (this.store.error) {
        StatusBanner({ text: this.store.error.message })
      }
      if (this.store.loading) {
        LoadingBlock({ text: '正在加载应用列表' })
      } else if (this.store.items.length === 0) {
        EmptyState({ text: '暂无已导入应用' })
        ImportEntryButton()
      } else {
        AppListView({ items: this.store.items })
      }
    }
    .width('100%')
    .height('100%')
  }
}
