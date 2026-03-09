import { AppCatalogStore } from '../store/AppCatalogStore'

@Entry
@Component
struct AppListPage {
  private store: AppCatalogStore = new AppCatalogStore()

  aboutToAppear(): void {
    this.store.loadApps()
  }

  build() {
    Column() {
      Text('AppListPage')
      Text(`apps: ${this.store.items.length}`)
    }
    .width('100%')
    .height('100%')
  }
}
