import { AppCatalogStore } from '../store/AppCatalogStore'
import { LoadingBlock } from '../components/common/LoadingBlock'
import { StatusBanner } from '../components/common/StatusBanner'
import type { AppItemKind, AppItemVM } from '../models/AppItemVM'

@ComponentV2
export struct RecentRunPage {
  @Param refreshToken: number = 0
  @Param recentAppIdsText: string = ''
  @Param scroller: Scroller = new Scroller()
  @Event
  onNavigateRun?: (appId: string) => void
  @Event
  onNavigateImport?: () => void
  @Event
  onClearRecent?: () => void

  @Local refreshTick: number = 0
  @Local clearArmed: boolean = false
  @Local items: AppItemVM[] = []
  @Local activeRecentAppIdsText: string = ''
  @Local recentItemsSnapshot: AppItemVM[] = []
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
    this.syncRecentText()
    void this.loadAppsSnapshot()
  }

  aboutToDisappear(): void {
    if (this.unsubscribe) {
      this.unsubscribe()
      this.unsubscribe = undefined
    }
  }

  @Monitor('recentAppIdsText')
  private onRecentAppIdsTextChange(): void {
    this.clearArmed = false
    this.syncRecentText()
  }

  @Monitor('refreshToken')
  private onRefreshTokenChange(): void {
    if (this.refreshToken === this.lastHandledRefreshToken) {
      return
    }
    this.lastHandledRefreshToken = this.refreshToken
    this.clearArmed = false
    void this.refreshAppsSnapshot()
  }

  private syncFromStore(): void {
    this.items = [...this.store.items]
    this.loading = this.store.loading
    this.errorMessage = this.store.error?.message ?? ''
    this.rebuildRecentItems()
    this.refreshTick += 1
  }

  private syncRecentText(): void {
    this.activeRecentAppIdsText = this.recentAppIdsText
    this.rebuildRecentItems()
    this.refreshTick += 1
  }

  private rebuildRecentItems(): void {
    const nextItems: AppItemVM[] = []
    for (const appId of this.recentIds()) {
      const item = this.items.find((candidate: AppItemVM) => candidate.id === appId)
      if (item) {
        nextItems.push(item)
      }
    }
    this.recentItemsSnapshot = nextItems
  }

  private async loadAppsSnapshot(): Promise<void> {
    await this.store.loadApps()
    this.syncFromStore()
  }

  private async refreshAppsSnapshot(): Promise<void> {
    await this.store.refreshApps()
    this.syncFromStore()
  }

  private recentIds(): string[] {
    if (this.activeRecentAppIdsText.length === 0) {
      return []
    }
    return this.activeRecentAppIdsText.split('|').filter((id: string) => id.length > 0)
  }

  private recentItems(): AppItemVM[] {
    return this.recentItemsSnapshot
  }

  private firstRecentItems(): AppItemVM[] {
    if (this.recentItemsSnapshot.length === 0) {
      return []
    }
    return [this.recentItemsSnapshot[0]]
  }

  private firstRecentItem(): AppItemVM | undefined {
    const items = this.recentItems()
    return items.length > 0 ? items[0] : undefined
  }

  private kindText(kind: AppItemKind): string {
    return kind === 'game' ? '游戏' : '应用'
  }

  private kindMark(kind: AppItemKind): string {
    return kind === 'game' ? 'MRP' : 'APP'
  }

  private kindTint(kind: AppItemKind): Resource {
    return kind === 'game' ? $r('app.color.app_game_text') : $r('app.color.app_app_text')
  }

  private kindTileStart(kind: AppItemKind): Resource {
    return kind === 'game' ? $r('app.color.app_game_tile_start') : $r('app.color.app_app_tile_start')
  }

  private kindTileEnd(kind: AppItemKind): Resource {
    return kind === 'game' ? $r('app.color.app_game_tile_end') : $r('app.color.app_app_tile_end')
  }

  private openRunPage(appId: string): void {
    this.clearArmed = false
    if (this.onNavigateRun) {
      this.onNavigateRun(appId)
    }
  }

  private openImportPage(): void {
    this.clearArmed = false
    if (this.onNavigateImport) {
      this.onNavigateImport()
    }
  }

  private clearRecent(): void {
    this.clearArmed = false
    if (this.onClearRecent) {
      this.onClearRecent()
    }
  }

  private requestClearRecent(): void {
    if (!this.clearArmed) {
      this.clearArmed = true
      return
    }
    this.clearRecent()
  }

  @Builder
  private TopSafeGap() {
    ListItem() {
      Column()
        .width('100%')
        .height(68)
    }
    .selectable(false)
  }

  @Builder
  private PackageIcon(item: AppItemVM, size: number) {
    Column() {
      if (item.icon) {
        Image(item.icon)
          .width(size - 10)
          .height(size - 10)
          .objectFit(ImageFit.Contain)
          .interpolation(ImageInterpolation.Medium)
      } else {
        Text(this.kindMark(item.kind))
          .fontSize(size >= 48 ? 11 : 9)
          .fontWeight(FontWeight.Bold)
          .fontColor(this.kindTint(item.kind))
      }
    }
    .width(size)
    .height(size)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .borderRadius(size >= 48 ? 15 : 11)
    .linearGradient({
      angle: 140,
      colors: [[this.kindTileStart(item.kind), 0.0], [this.kindTileEnd(item.kind), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
  }

  @Builder
  private RunPill(label: string, onTap: () => void) {
    Text(label)
      .fontSize(14)
      .fontWeight(FontWeight.Medium)
      .fontColor($r('app.color.app_accent_text'))
      .textAlign(TextAlign.Center)
      .height(36)
      .padding({ left: 16, right: 16 })
      .borderRadius(18)
      .backgroundColor($r('app.color.app_surface_muted'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .onClick(onTap)
  }

  @Builder
  private ContinueCard(item: AppItemVM) {
    ListItem() {
      Row({ space: 14 }) {
        this.PackageIcon(item, 54)

        Column({ space: 5 }) {
          Text('继续上次')
            .fontSize(12)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          Text(item.name)
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
          Text(`${this.kindText(item.kind)} · ${item.fileName}`)
            .fontSize(12)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        this.RunPill('运行', () => this.openRunPage(item.id))
      }
      .width('100%')
      .padding({ left: 16, right: 14, top: 16, bottom: 16 })
      .alignItems(VerticalAlign.Center)
      .linearGradient({
        angle: 115,
        colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 0.58], [$r('app.color.app_surface_muted'), 1.0]]
      })
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(24)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 10 })
    }
    .selectable(false)
  }

  @Builder
  private RecentRowsCard() {
    ListItem() {
      Column({ space: 0 }) {
        Row() {
          Column({ space: 3 }) {
            Text('最近记录')
              .fontSize(18)
              .fontWeight(FontWeight.Medium)
              .fontColor($r('sys.color.ohos_id_color_text_primary'))
            Text(`${this.recentItems().length} 个项目`)
              .fontSize(12)
              .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          }
          .layoutWeight(1)
          .alignItems(HorizontalAlign.Start)

          if (this.recentItems().length > 0) {
            Text(this.clearArmed ? '确认清空' : '清空')
              .fontSize(13)
              .fontWeight(FontWeight.Medium)
              .fontColor(this.clearArmed ? $r('app.color.app_danger_text') : $r('app.color.app_accent_text'))
              .height(32)
              .padding({ left: 12, right: 12 })
              .borderRadius(16)
              .backgroundColor(this.clearArmed ? $r('app.color.app_danger_soft') : $r('app.color.app_surface_muted'))
              .border({ width: 1, color: $r('app.color.app_surface_border') })
              .onClick(() => this.requestClearRecent())
          }
        }
        .width('100%')
        .padding({ left: 14, right: 12, top: 14, bottom: 10 })
        .alignItems(VerticalAlign.Center)

        if (this.clearArmed) {
          Text('再次点击确认清空，不会删除已导入的 MRP。')
            .fontSize(12)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .width('100%')
            .padding({ left: 14, right: 14, top: 8, bottom: 10 })
        }

        ForEach(this.recentItems(), (item: AppItemVM, index: number) => {
          this.RecentRow(item, index)
        }, (item: AppItemVM) => item.id)
      }
      .width('100%')
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(22)
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private RecentRow(item: AppItemVM, index: number) {
    Column() {
      Row({ space: 10 }) {
        this.PackageIcon(item, 38)

        Column({ space: 3 }) {
          Text(item.name)
            .fontSize(14)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
          Text(item.fileName)
            .fontSize(11)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        Text(index === 0 ? '最近' : this.kindText(item.kind))
          .fontSize(11)
          .fontColor(index === 0 ? $r('app.color.app_accent_text') : this.kindTint(item.kind))
          .height(26)
          .padding({ left: 9, right: 9 })
          .borderRadius(13)
          .backgroundColor($r('app.color.app_surface_muted'))
      }
      .width('100%')
      .height(58)
      .padding({ left: 12, right: 12 })
      .alignItems(VerticalAlign.Center)

      if (index < this.recentItems().length - 1) {
        Row() {
          Text('')
            .height(1)
            .layoutWeight(1)
            .backgroundColor($r('sys.color.ohos_id_color_list_separator'))
        }
        .width('100%')
        .padding({ left: 60 })
      }
    }
    .width('100%')
    .onClick(() => this.openRunPage(item.id))
  }

  @Builder
  private EmptyRecent() {
    ListItem() {
      Column({ space: 16 }) {
        Column() {
          Image($r('sys.media.ohos_ic_public_clock'))
            .width(34)
            .height(34)
            .opacity(0.78)
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

        Column({ space: 7 }) {
          Text('暂无最近运行')
            .fontSize(19)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
          Text('打开一个 MRP 后会显示在这里。')
            .fontSize(13)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .textAlign(TextAlign.Center)
        }
        .alignItems(HorizontalAlign.Center)

        this.RunPill('导入 MRP', () => this.openImportPage())
      }
      .width('100%')
      .padding({ left: 24, right: 24, top: 40, bottom: 40 })
      .alignItems(HorizontalAlign.Center)
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .borderRadius(24)
      .margin({ top: 2, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private BottomSafeTail() {
    ListItem() {
      Column()
        .width('100%')
        .height(146)
    }
    .selectable(false)
  }

  build() {
    List({ scroller: this.scroller }) {
      this.TopSafeGap()
      ListItem() {
        Text(`${this.refreshTick}-${this.refreshToken}-${this.activeRecentAppIdsText}`).fontSize(1).opacity(0)
      }
      .height(1)
      .selectable(false)
      if (this.errorMessage.length > 0) {
        ListItem() {
          StatusBanner({ text: this.errorMessage })
        }
        .selectable(false)
      }
      if (this.loading && this.items.length === 0) {
        ListItem() {
          LoadingBlock({ text: '正在加载最近记录' })
        }
        .selectable(false)
      } else if (this.recentItems().length === 0) {
        this.EmptyRecent()
      } else {
        ForEach(this.firstRecentItems(), (item: AppItemVM) => {
          this.ContinueCard(item)
        }, (item: AppItemVM) => `${this.activeRecentAppIdsText}-${item.id}`)
        this.RecentRowsCard()
      }
      this.BottomSafeTail()
    }
    .width('100%')
    .height('100%')
    .padding({ left: 16, right: 16 })
    .edgeEffect(EdgeEffect.Spring)
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
