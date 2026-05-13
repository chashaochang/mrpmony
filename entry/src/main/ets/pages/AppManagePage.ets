import promptAction from '@ohos.promptAction'
import { AppCatalogStore } from '../store/AppCatalogStore'
import { LoadingBlock } from '../components/common/LoadingBlock'
import { StatusBanner } from '../components/common/StatusBanner'
import type { AppItemKind, AppItemVM } from '../models/AppItemVM'

@ComponentV2
export struct AppManagePage {
  @Param refreshToken: number = 0
  @Param scroller: Scroller = new Scroller()
  @Event
  onAppsDeleted?: (appIdsText: string) => void

  @Local refreshTick: number = 0
  @Local selectedIdsText: string = ''
  @Local items: AppItemVM[] = []
  @Local loading: boolean = false
  @Local errorMessage: string = ''
  private lastHandledRefreshToken: number = -1
  private store: AppCatalogStore = AppCatalogStore.shared()
  private unsubscribe?: () => void
  private selectedCacheText: string = ''
  private selectedCacheIds: string[] = []
  private selectedCacheMap: Record<string, boolean> = {}

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

  @Monitor('refreshToken')
  private onRefreshTokenChange(): void {
    if (this.refreshToken === this.lastHandledRefreshToken) {
      return
    }
    this.lastHandledRefreshToken = this.refreshToken
    this.selectedIdsText = ''
    void this.refreshAppsSnapshot()
  }

  private syncFromStore(): void {
    this.items = [...this.store.items]
    this.loading = this.store.loading
    this.errorMessage = this.store.error?.message ?? ''
    this.pruneSelection()
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

  private pruneSelection(): void {
    if (this.selectedIdsText.length === 0) {
      return
    }
    const itemMap: Record<string, boolean> = {}
    for (const item of this.items) {
      itemMap[item.id] = true
    }
    const nextIds = this.selectedIds().filter((id: string) => itemMap[id] === true)
    const nextText = nextIds.join('|')
    if (nextText !== this.selectedIdsText) {
      this.selectedIdsText = nextText
    }
  }

  private selectedIds(): string[] {
    this.ensureSelectionCache()
    return this.selectedCacheIds
  }

  private ensureSelectionCache(): void {
    if (this.selectedCacheText === this.selectedIdsText) {
      return
    }
    this.selectedCacheText = this.selectedIdsText
    if (this.selectedIdsText.length === 0) {
      this.selectedCacheIds = []
      this.selectedCacheMap = {}
      return
    }
    const ids = this.selectedIdsText.split('|').filter((id: string) => id.length > 0)
    const map: Record<string, boolean> = {}
    for (const id of ids) {
      map[id] = true
    }
    this.selectedCacheIds = ids
    this.selectedCacheMap = map
  }

  private selectedCount(): number {
    this.ensureSelectionCache()
    return this.selectedCacheIds.length
  }

  private isSelected(appId: string): boolean {
    this.ensureSelectionCache()
    return this.selectedCacheMap[appId] === true
  }

  private toggleSelected(appId: string): void {
    const ids = this.selectedIds()
    const exists = ids.includes(appId)
    this.selectedIdsText = exists ? ids.filter((id: string) => id !== appId).join('|') : [...ids, appId].join('|')
  }

  private selectAll(): void {
    if (this.selectedCount() === this.items.length) {
      this.selectedIdsText = ''
      return
    }
    this.selectedIdsText = this.items.map((item: AppItemVM) => item.id).join('|')
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

  private sizeText(sizeBytes?: number): string {
    if (sizeBytes === undefined) {
      return '未知大小'
    }
    if (sizeBytes >= 1024 * 1024) {
      return `${(sizeBytes / 1024 / 1024).toFixed(2)} MB`
    }
    if (sizeBytes >= 1024) {
      return `${(sizeBytes / 1024).toFixed(1)} KB`
    }
    return `${sizeBytes} B`
  }

  private async deleteSelected(): Promise<void> {
    const ids = this.selectedIds()
    if (ids.length === 0) {
      return
    }
    try {
      const result = await promptAction.showDialog({
        title: `删除 ${ids.length} 个 MRP？`,
        message: '可以只删除 MRP 文件，也可以同时清理所选 MRP 的设置和匹配到的本地数据。',
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
      const deletedIds = await this.store.deleteApps(ids, deleteRelatedData)
      this.syncFromStore()
      if (deletedIds.length === 0) {
        promptAction.showToast({ message: this.errorMessage.length > 0 ? this.errorMessage : '删除失败' })
        return
      }
      this.selectedIdsText = ''
      if (this.onAppsDeleted) {
        this.onAppsDeleted(deletedIds.join('|'))
      }
      promptAction.showToast({ message: deleteRelatedData ? `已删除 ${deletedIds.length} 个 MRP 和数据` : `已删除 ${deletedIds.length} 个 MRP` })
    } catch (_error) {
    }
  }

  @Builder
  private TopSafeGap() {
    ListItem() {
      Column()
        .width('100%')
        .height(78)
    }
    .selectable(false)
  }

  @Builder
  private CheckMark(selected: boolean) {
    Column() {
      if (selected) {
        Text('')
          .width(10)
          .height(10)
          .borderRadius(5)
          .backgroundColor($r('app.color.app_accent_text'))
      }
    }
    .width(24)
    .height(24)
    .borderRadius(12)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .backgroundColor(selected ? $r('app.color.app_primary_soft') : $r('app.color.app_surface_muted'))
    .border({ width: 1, color: selected ? $r('app.color.app_accent_text') : $r('app.color.app_surface_border') })
  }

  @Builder
  private PackageIcon(item: AppItemVM) {
    Column() {
      if (item.icon) {
        Image(item.icon)
          .width(30)
          .height(30)
          .objectFit(ImageFit.Contain)
          .interpolation(ImageInterpolation.Medium)
      } else {
        Text(this.kindMark(item.kind))
          .fontSize(9)
          .fontWeight(FontWeight.Bold)
          .fontColor(this.kindTint(item.kind))
      }
    }
    .width(38)
    .height(38)
    .borderRadius(11)
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
    .linearGradient({
      angle: 140,
      colors: [[this.kindTileStart(item.kind), 0.0], [this.kindTileEnd(item.kind), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
  }

  @Builder
  private Toolbar() {
    ListItem() {
      Row({ space: 10 }) {
        Text(this.selectedCount() > 0 ? `已选 ${this.selectedCount()} 个` : `共 ${this.items.length} 个`)
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
          .layoutWeight(1)

        Text(this.selectedCount() === this.items.length && this.items.length > 0 ? '取消全选' : '全选')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('app.color.app_accent_text'))
          .height(34)
          .padding({ left: 14, right: 14 })
          .borderRadius(17)
          .backgroundColor($r('app.color.app_surface_muted'))
          .border({ width: 1, color: $r('app.color.app_surface_border') })
          .onClick(() => this.selectAll())

        Text('删除所选')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor(this.selectedCount() > 0 ? $r('app.color.app_danger_text') : $r('sys.color.ohos_id_color_text_tertiary'))
          .height(34)
          .padding({ left: 14, right: 14 })
          .borderRadius(17)
          .backgroundColor(this.selectedCount() > 0 ? $r('app.color.app_danger_soft') : $r('app.color.app_surface_muted'))
          .border({ width: 1, color: $r('app.color.app_surface_border') })
          .onClick(() => {
            void this.deleteSelected()
          })
      }
      .width('100%')
      .padding({ left: 14, right: 14, top: 12, bottom: 12 })
      .borderRadius(22)
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .shadow({
        radius: 18,
        color: '#24385022',
        offsetX: 0,
        offsetY: 8
      })
      .margin({ bottom: 10 })
    }
    .selectable(false)
  }

  @Builder
  private AppRowItem(item: AppItemVM) {
    ListItem() {
      Row({ space: 10 }) {
        this.PackageIcon(item)
        Column({ space: 3 }) {
          Text(item.name)
            .fontSize(14)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
          Text(`${item.fileName} · ${this.sizeText(item.sizeBytes)}`)
            .fontSize(11)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .width('100%')
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        this.CheckMark(this.isSelected(item.id))
      }
      .width('100%')
      .height(62)
      .padding({ left: 12, right: 12 })
      .alignItems(VerticalAlign.Center)
      .backgroundColor($r('app.color.app_surface'))
      .borderRadius(18)
      .border({ width: 1, color: this.isSelected(item.id) ? $r('app.color.app_accent_text') : $r('app.color.app_surface_border') })
      .shadow({
        radius: this.isSelected(item.id) ? 16 : 10,
        color: this.isSelected(item.id) ? '#24385024' : '#24385014',
        offsetX: 0,
        offsetY: this.isSelected(item.id) ? 7 : 4
      })
      .margin({ bottom: 8 })
      .onClick(() => this.toggleSelected(item.id))
    }
    .selectable(false)
  }

  @Builder
  private EmptyState() {
    ListItem() {
      Column({ space: 8 }) {
        Text('没有可管理的 MRP')
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
        Text('导入后可以在这里批量整理。')
          .fontSize(13)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
      }
      .width('100%')
      .padding({ left: 24, right: 24, top: 46, bottom: 46 })
      .alignItems(HorizontalAlign.Center)
      .borderRadius(24)
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
    }
    .selectable(false)
  }

  @Builder
  private BottomSafeTail() {
    ListItem() {
      Column()
        .width('100%')
        .height(44)
    }
    .selectable(false)
  }

  build() {
    List({ scroller: this.scroller }) {
      this.TopSafeGap()
      ListItem() {
        Text(`${this.refreshTick}-${this.refreshToken}-${this.selectedIdsText}`).fontSize(1).opacity(0)
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
          LoadingBlock({ text: '正在加载管理列表' })
        }
        .selectable(false)
      } else if (this.items.length === 0) {
        this.EmptyState()
      } else {
        this.Toolbar()
        ForEach(this.items, (item: AppItemVM) => {
          this.AppRowItem(item)
        }, (item: AppItemVM) => item.id)
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
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.BOTTOM])
  }
}
