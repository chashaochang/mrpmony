import { AppListPage } from './AppListPage'
import { ImportPage } from './ImportPage'
import { RunPage } from './RunPage'

export type ShellRoute = 'appList' | 'import' | 'run'

@Entry
@Component
struct ShellRootPage {
  @State currentRoute: ShellRoute = 'appList'
  @State currentAppId: string = ''
  @State catalogRefreshTick: number = 0

  private goToImport(): void {
    this.currentRoute = 'import'
  }

  private goToRun(appId: string): void {
    this.currentAppId = appId
    this.currentRoute = 'run'
  }

  private backToList(): void {
    this.currentRoute = 'appList'
  }

  private backToListAfterImport(): void {
    this.catalogRefreshTick += 1
    this.currentRoute = 'appList'
  }

  build() {
    Column() {
      Text(`${this.catalogRefreshTick}`).fontSize(1).opacity(0)
      if (this.currentRoute === 'appList') {
        AppListPage({
          refreshToken: this.catalogRefreshTick,
          onNavigateImport: () => this.goToImport(),
          onNavigateRun: (appId: string) => this.goToRun(appId)
        })
      }
      if (this.currentRoute === 'import') {
        ImportPage({
          onBack: () => this.backToList(),
          onImportSuccess: () => this.backToListAfterImport()
        })
      }
      if (this.currentRoute === 'run') {
        RunPage({
          appId: this.currentAppId,
          onBack: () => this.backToList()
        })
      }
    }
    .width('100%')
    .height('100%')
  }
}
