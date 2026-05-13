import promptAction from '@ohos.promptAction'
import { AppCatalogStore } from '../store/AppCatalogStore'
import { LoadingBlock } from '../components/common/LoadingBlock'
import { StatusBanner } from '../components/common/StatusBanner'
import { AppListView } from '../components/appList/AppListView'
import { ImportEntryButton } from '../components/appList/ImportEntryButton'
import type { AppItemVM } from '../models/AppItemVM'

@ComponentV2
export struct AppListPage {
  @Param refreshToken: number = 0
  @Param recentAppIdsText: string = ''
  @Param showImportEntryButton: boolean = true
  @Param scroller: Scroller = new Scroller()
  @Param topSafeGap: number = 0
  @Event
  onNavigateImport?: () => void
  @Event
  onNavigateRun?: (appId: string) => void
  @Event
  onAppDeleted?: (appId: string) => void

  @Local refreshTick: number = 0
  @Local items: AppItemVM[] = []
  @Local loading: boolean = false
  @Local errorMessage: string = ''
  private lastHandledRefreshToken: number = -1
  private store: AppCatalogStore = AppCatalogStore.shared()
  private unsubscribe?: () => void

  aboutToAppear(): void {
    if (!this.unsubscribe) {
      this.unsubscribe = this.store.subscribe(() => {
        this.syncFromStore()
      })
    }
    this.syncFromStore()
    void this.loadAppsSnapshot()
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
    void this.refreshAppsSnapshot()
  }

  private syncFromStore(): void {
    this.items = [...this.store.items]
    this.loading = this.store.loading
    this.errorMessage = this.store.error?.message ?? ''
    this.refreshTick += 1
  }

  private async loadAppsSnapshot(): Promise<void> {
    await this.store.loadApps()
    this.syncFromStore()
  }

  private async refreshAppsSnapshot(): Promise<void> {
    await this.store.refreshApps()
    this.syncFromStore()
  }

  @Monitor('refreshToken')
  private onRefreshTokenChange(): void {
    this.ensureRefreshByToken()
  }

  private openImportPage(): void {
    if (this.onNavigateImport) {
      this.onNavigateImport()
      return
    }
    console.info('TODO: open ImportPage')
  }

  private openRunPage(appId: string): void {
    if (this.onNavigateRun) {
      this.onNavigateRun(appId)
      return
    }
    console.info(`TODO: open RunPage for ${appId}`)
  }

  private sizeText(sizeBytes?: number): string {
    if (sizeBytes === undefined) {
      return '未知'
    }
    if (sizeBytes >= 1024 * 1024) {
      return `${(sizeBytes / 1024 / 1024).toFixed(2)} MB`
    }
    if (sizeBytes >= 1024) {
      return `${(sizeBytes / 1024).toFixed(1)} KB`
    }
    return `${sizeBytes} B`
  }

  private kindText(item: AppItemVM): string {
    return item.kind === 'game' ? '游戏' : '应用'
  }

  private async showAppDetail(appId: string): Promise<void> {
    const item = this.items.find((candidate: AppItemVM) => candidate.id === appId)
    if (!item) {
      return
    }
    try {
      await promptAction.showDialog({
        title: item.name,
        message: `文件名：${item.fileName}\n类型：${this.kindText(item)}\n大小：${this.sizeText(item.sizeBytes)}\n状态：${item.status === 'ready' ? '可运行' : '不可运行'}`,
        buttons: [
          {
            text: '知道了',
            color: $r('app.color.app_accent_text'),
            primary: true
          }
        ]
      })
    } catch (_error) {
    }
  }

  private async showItemActionMenu(appId: string): Promise<void> {
    const item = this.items.find((candidate: AppItemVM) => candidate.id === appId)
    if (!item) {
      return
    }
    try {
      const result = await promptAction.showActionMenu({
        title: item.name,
        buttons: [
          {
            text: '运行',
            color: $r('sys.color.ohos_id_color_text_primary')
          },
          {
            text: '详细信息',
            color: $r('sys.color.ohos_id_color_text_primary')
          },
          {
            text: '删除',
            color: $r('app.color.app_danger_text')
          }
        ]
      })
      if (result.index === 0) {
        this.openRunPage(appId)
      } else if (result.index === 1) {
        await this.showAppDetail(appId)
      } else if (result.index === 2) {
        await this.confirmDeleteApp(appId)
      }
    } catch (_error) {
    }
  }

  private async confirmDeleteApp(appId: string): Promise<void> {
    const item = this.items.find((candidate: AppItemVM) => candidate.id === appId)
    if (!item) {
      return
    }
    try {
      const result = await promptAction.showDialog({
        title: '删除 MRP？',
        message: `将从本机移除“${item.name}”。可以只删除 MRP 文件，也可以同时清理该 MRP 的设置和匹配到的本地数据。`,
        buttons: [
          {
            text: '取消',
            color: $r('sys.color.ohos_id_color_text_primary')
          },
          {
            text: '仅删除 MRP',
            color: $r('app.color.app_danger_text')
          },
          {
            text: '删除 MRP 和数据',
            color: $r('app.color.app_danger_text'),
            primary: true
          }
        ]
      })
      if (result.index === 0) {
        return
      }
      const deleteRelatedData = result.index === 2
      const deleted = await this.store.deleteApp(appId, deleteRelatedData)
      this.syncFromStore()
      if (!deleted) {
        promptAction.showToast({ message: this.errorMessage.length > 0 ? this.errorMessage : '删除失败' })
        return
      }
      if (this.onAppDeleted) {
        this.onAppDeleted(appId)
      }
      promptAction.showToast({ message: deleteRelatedData ? '已删除 MRP 和数据' : '已删除 MRP' })
    } catch (_error) {
      // User dismissed the dialog.
    }
  }

  @Builder
  private TopSafeGap() {
    if (this.topSafeGap > 0) {
      Column()
        .width('100%')
        .height(this.topSafeGap)
    }
  }

  @Builder
  private ProductEmptyState() {
    Column({ space: 16 }) {
      Column() {
        Image($r('sys.media.ohos_ic_normal_white_grid_folder'))
          .width(34)
          .height(34)
          .opacity(0.84)
      }
      .width(62)
      .height(62)
      .borderRadius(18)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .linearGradient({
        angle: 135,
        colors: [[$r('app.color.app_surface_muted'), 0.0], [$r('app.color.app_primary_soft'), 1.0]]
      })
      .border({ width: 1, color: $r('app.color.app_surface_border') })

      Column({ space: 8 }) {
        Text('本地库为空')
          .fontSize(21)
          .fontWeight(FontWeight.Bold)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
        Text('导入 .mrp 文件后会显示在这里。')
          .fontSize(13)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          .textAlign(TextAlign.Center)
          .lineHeight(20)
          .maxLines(3)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .alignItems(HorizontalAlign.Center)

      Row({ space: 8 }) {
        Text('+')
          .fontSize(22)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('app.color.app_accent_text'))
          .textAlign(TextAlign.Center)
          .width(18)
          .height(22)
        Text('导入 MRP 文件')
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
      }
        .height(44)
        .padding({ left: 18, right: 18 })
        .borderRadius(16)
        .backgroundColor($r('app.color.app_surface'))
        .border({ width: 1, color: $r('app.color.app_surface_border') })
        .alignItems(VerticalAlign.Center)
        .onClick(() => this.openImportPage())
    }
    .width('100%')
    .layoutWeight(1)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .padding({ left: 28, right: 28, bottom: 132 })
  }

  build() {
    Column() {
      Text(`${this.refreshTick}-${this.refreshToken}-${this.recentAppIdsText}`).fontSize(1).opacity(0)
      if (this.loading && this.items.length === 0) {
        this.TopSafeGap()
        if (this.errorMessage.length > 0) {
          StatusBanner({ text: this.errorMessage })
        }
        LoadingBlock({ text: '正在加载应用列表' })
      } else if (this.items.length === 0) {
        this.TopSafeGap()
        if (this.errorMessage.length > 0) {
          StatusBanner({ text: this.errorMessage })
        }
        this.ProductEmptyState()
        if (this.showImportEntryButton) {
          ImportEntryButton({ onTap: () => this.openImportPage() })
        }
      } else {
        if (this.errorMessage.length > 0) {
          this.TopSafeGap()
          StatusBanner({ text: this.errorMessage })
        }
        AppListView({
          items: this.items,
          recentAppIdsText: this.recentAppIdsText,
          scroller: this.scroller,
          showSearch: true,
          topSafeGap: this.errorMessage.length > 0 ? 12 : this.topSafeGap,
          onItemTap: (appId: string) => this.openRunPage(appId),
          onItemLongPress: (appId: string) => {
            void this.showItemActionMenu(appId)
          }
        })
        if (this.showImportEntryButton) {
          ImportEntryButton({ onTap: () => this.openImportPage() })
        }
      }
    }
    .width('100%')
    .height('100%')
    .padding({ top: 8 })
    .linearGradient({
      angle: 180,
      colors: [
        [$r('app.color.home_background_start'), 0.0],
        [$r('app.color.home_background_mid'), 0.48],
        [$r('app.color.home_background_end'), 1.0]
      ]
    })
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.TOP, SafeAreaEdge.BOTTOM])
  }
}
