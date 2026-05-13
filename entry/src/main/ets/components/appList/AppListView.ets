import type { AppItemKind, AppItemVM } from '../../models/AppItemVM'
import { AppPreferences } from '../../services/AppPreferences'
import type { ListSortMode } from '../../services/AppPreferences'

type AppListFilter = 'all' | AppItemKind
type AppListSortMode = ListSortMode | 'input'

interface PopupStateChange {
  isVisible: boolean
}

@ComponentV2
export struct AppListView {
  @Param items: AppItemVM[] = []
  @Param recentAppIdsText: string = ''
  @Param showSearch: boolean = true
  @Param showFilters: boolean = true
  @Param showHeader: boolean = true
  @Param sortMode: AppListSortMode = 'name'
  @Param scroller: Scroller = new Scroller()
  @Param externalSearchQuery: string = ''
  @Param topSafeGap: number = 0
  @Event
  onItemTap?: (appId: string) => void
  @Event
  onItemLongPress?: (appId: string) => void

  @Local selectedFilter: AppListFilter = 'all'
  @Local localSearchQuery: string = ''
  @Local activeSortMode: ListSortMode = 'name'
  @Local sortRefreshTick: number = 0
  @Local sortMenuVisible: boolean = false
  private appPreferences: AppPreferences = new AppPreferences()
  private recentCacheText: string = ''
  private recentCacheIds: string[] = []
  private recentCacheIndexById: Record<string, number> = {}

  aboutToAppear(): void {
    if (this.sortMode === 'input') {
      return
    }
    this.activeSortMode = this.sortMode === 'name' ? this.appPreferences.listSortMode() : this.sortMode
  }

  @Monitor('recentAppIdsText')
  private onRecentAppIdsTextChange(): void {
    this.recentCacheText = ''
    this.recentCacheIds = []
    this.recentCacheIndexById = {}
    this.sortRefreshTick += 1
  }

  private filteredItems(kind: AppItemKind): AppItemVM[] {
    return this.searchedItems().filter((item: AppItemVM) => item.kind === kind)
  }

  private visibleItems(): AppItemVM[] {
    if (this.selectedFilter === 'all') {
      return this.searchedItems()
    }
    return this.filteredItems(this.selectedFilter)
  }

  private filterText(filter: AppListFilter): string {
    if (filter === 'all') {
      return `全部 ${this.searchedItems().length}`
    }
    return `${this.kindTitle(filter)} ${this.filteredItems(filter).length}`
  }

  private isFilterSelected(filter: AppListFilter): boolean {
    return this.selectedFilter === filter
  }

  private kindTitle(kind: AppItemKind): string {
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

  private recentAppIds(): string[] {
    this.ensureRecentCache()
    return this.recentCacheIds
  }

  private ensureRecentCache(): void {
    if (this.recentCacheText === this.recentAppIdsText) {
      return
    }
    this.recentCacheText = this.recentAppIdsText
    if (this.recentAppIdsText.length === 0) {
      this.recentCacheIds = []
      this.recentCacheIndexById = {}
      return
    }
    const ids = this.recentAppIdsText.split('|').filter((id: string) => id.length > 0)
    const indexById: Record<string, number> = {}
    for (let index = 0; index < ids.length; index++) {
      indexById[ids[index]] = index
    }
    this.recentCacheIds = ids
    this.recentCacheIndexById = indexById
  }

  private recentOrderMap(): Record<string, number> {
    this.ensureRecentCache()
    return this.recentCacheIndexById
  }

  private recentIndex(appId: string): number {
    const index = this.recentOrderMap()[appId]
    return index === undefined ? -1 : index
  }

  private recentLabel(item: AppItemVM): string {
    const index = this.recentIndex(item.id)
    if (index === 0) {
      return '最近'
    }
    if (index > 0) {
      return '运行过'
    }
    return ''
  }

  private effectiveSortMode(): AppListSortMode {
    if (this.sortMode === 'input') {
      return 'input'
    }
    return this.activeSortMode
  }

  private sortModeText(): string {
    const mode = this.effectiveSortMode()
    if (mode === 'added') {
      return '加入时间'
    }
    if (mode === 'recent') {
      return '最近打开'
    }
    return '按名称'
  }

  private selectSortMode(mode: ListSortMode): void {
    if (this.sortMode === 'input') {
      return
    }
    this.sortMenuVisible = false
    if (this.activeSortMode === mode) {
      return
    }
    this.activeSortMode = mode
    this.sortRefreshTick += 1
    this.appPreferences.saveListSortMode(this.activeSortMode)
  }

  private emptyText(): string {
    if (this.normalizedSearchQuery().length > 0) {
      return `没有找到“${this.effectiveSearchQuery()}”`
    }
    if (this.selectedFilter === 'game') {
      return '暂无游戏'
    }
    if (this.selectedFilter === 'app') {
      return '暂无应用'
    }
    return '暂无内容'
  }

  private normalizedSearchQuery(): string {
    return this.effectiveSearchQuery().trim().toLowerCase()
  }

  private effectiveSearchQuery(): string {
    if (this.showSearch) {
      return this.localSearchQuery
    }
    return this.externalSearchQuery
  }

  private searchedItems(): AppItemVM[] {
    const query = this.normalizedSearchQuery()
    const matched = query.length === 0
      ? this.items
      : this.items.filter((item: AppItemVM) => this.matchesSearch(item, query))
    const mode = this.effectiveSortMode()
    if (mode === 'input') {
      return matched
    }
    if (mode === 'added') {
      return [...matched].sort((left: AppItemVM, right: AppItemVM) => {
        const leftAddedAt = left.addedAt ?? 0
        const rightAddedAt = right.addedAt ?? 0
        if (leftAddedAt !== rightAddedAt) {
          return rightAddedAt - leftAddedAt
        }
        return left.name.localeCompare(right.name)
      })
    }
    if (mode === 'recent') {
      const recentOrder = this.recentOrderMap()
      return [...matched].sort((left: AppItemVM, right: AppItemVM) => {
        const leftIndex = recentOrder[left.id]
        const rightIndex = recentOrder[right.id]
        const leftInRecent = leftIndex !== undefined
        const rightInRecent = rightIndex !== undefined
        if (leftInRecent && rightInRecent) {
          return leftIndex - rightIndex
        }
        if (leftInRecent) {
          return -1
        }
        if (rightInRecent) {
          return 1
        }
        return left.name.localeCompare(right.name)
      })
    }
    return [...matched].sort((left: AppItemVM, right: AppItemVM) => left.name.localeCompare(right.name))
  }

  private matchesSearch(item: AppItemVM, query: string): boolean {
    const source = `${item.name} ${item.fileName} ${item.description ?? ''}`.toLowerCase()
    return source.includes(query)
  }

  @Builder
  private FilterButton(filter: AppListFilter) {
    Column({ space: 3 }) {
      Text(this.filterText(filter))
        .fontSize(13)
        .fontWeight(this.isFilterSelected(filter) ? FontWeight.Medium : FontWeight.Regular)
        .fontColor(this.isFilterSelected(filter) ? $r('sys.color.ohos_id_color_text_primary') : $r('sys.color.ohos_id_color_text_secondary'))
        .textAlign(TextAlign.Center)
        .maxLines(1)

      Text('')
        .width(this.isFilterSelected(filter) ? 18 : 0)
        .height(2)
        .borderRadius(1)
        .backgroundColor(this.isFilterSelected(filter) ? $r('sys.color.ohos_id_color_emphasize') : $r('sys.color.ohos_id_color_titlebar_bg_transparent'))
    }
      .layoutWeight(1)
      .height(34)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .borderRadius(17)
      .linearGradient({
        angle: 130,
        colors: this.isFilterSelected(filter)
          ? [[$r('app.color.app_surface_muted'), 0.0], [$r('app.color.app_primary_soft'), 1.0]]
          : [[$r('sys.color.ohos_id_color_titlebar_bg_transparent'), 0.0], [$r('sys.color.ohos_id_color_titlebar_bg_transparent'), 1.0]]
      })
      .border({ width: this.isFilterSelected(filter) ? 1 : 0, color: $r('app.color.app_surface_border') })
      .onClick(() => {
        this.selectedFilter = filter
      })
  }

  @Builder
  private FilterBar() {
    ListItem() {
      Row({ space: 2 }) {
        this.FilterButton('all')
        this.FilterButton('game')
        this.FilterButton('app')
      }
      .width('100%')
      .height(38)
      .padding({ left: 2, right: 2 })
      .borderRadius(19)
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .shadow({
        radius: 14,
        color: '#24385018',
        offsetX: 0,
        offsetY: 6
      })
      .margin({ top: 0, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private SearchBar() {
    ListItem() {
      Row({ space: 10 }) {
        Image($r('sys.media.ohos_ic_public_search_filled'))
          .width(18)
          .height(18)
          .fillColor($r('app.color.app_accent_text'))
          .opacity(0.72)

        TextInput({ placeholder: '搜索 MRP', text: this.localSearchQuery })
          .fontSize(15)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
          .placeholderColor($r('sys.color.ohos_id_color_text_hint'))
          .caretColor($r('sys.color.ohos_id_color_emphasize'))
          .height(40)
          .layoutWeight(1)
          .backgroundColor($r('sys.color.ohos_id_color_titlebar_bg_transparent'))
          .padding({ left: 0, right: 0 })
          .onChange((value: string) => {
            this.localSearchQuery = value
          })

        if (this.normalizedSearchQuery().length > 0) {
          Column() {
            Image($r('sys.media.ohos_ic_public_input_cancel'))
              .width(16)
              .height(16)
              .fillColor($r('sys.color.ohos_id_color_text_secondary'))
              .opacity(0.68)
          }
            .width(30)
            .height(30)
            .borderRadius(15)
            .justifyContent(FlexAlign.Center)
            .alignItems(HorizontalAlign.Center)
            .backgroundColor($r('sys.color.ohos_id_color_click_effect'))
            .onClick(() => {
              this.localSearchQuery = ''
            })
        }
      }
      .width('100%')
      .height(48)
      .padding({ left: 14, right: 10, top: 4, bottom: 4 })
      .borderRadius(24)
      .backgroundColor($r('app.color.app_surface'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .shadow({
        radius: 14,
        color: '#24385018',
        offsetX: 0,
        offsetY: 6
      })
      .margin({ top: 0, bottom: 8 })
    }
    .selectable(false)
  }

  @Builder
  private TopSafeGap() {
    if (this.topSafeGap > 0) {
      ListItem() {
        Column()
          .width('100%')
          .height(this.topSafeGap)
      }
      .selectable(false)
    }
  }

  @Builder
  private SortOptionRow(mode: ListSortMode, title: string) {
    Row({ space: 9 }) {
      Column() {
        Text('')
          .width(this.activeSortMode === mode ? 4 : 0)
          .height(20)
          .borderRadius(2)
          .backgroundColor(this.activeSortMode === mode ? $r('app.color.app_accent_text') : $r('sys.color.ohos_id_color_titlebar_bg_transparent'))
      }
      .width(8)
      .height(30)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)

      Text(title)
        .fontSize(14)
        .fontWeight(this.activeSortMode === mode ? FontWeight.Medium : FontWeight.Regular)
        .fontColor(this.activeSortMode === mode ? $r('sys.color.ohos_id_color_text_primary') : $r('sys.color.ohos_id_color_text_secondary'))
        .maxLines(1)
      .layoutWeight(1)
    }
    .width('100%')
    .height(42)
    .padding({ left: 9, right: 8 })
    .borderRadius(14)
    .backgroundColor(this.activeSortMode === mode ? $r('app.color.app_surface_muted') : $r('sys.color.ohos_id_color_titlebar_bg_transparent'))
    .onClick(() => this.selectSortMode(mode))
  }

  @Builder
  private SortMenu() {
    Column({ space: 5 }) {
      Row() {
        Text('排序方式')
          .fontSize(12)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          .maxLines(1)
      }
      .width('100%')
      .padding({ left: 12, right: 12, top: 4, bottom: 0 })

      this.SortOptionRow('name', '按名称')
      this.SortOptionRow('added', '最新加入')
      this.SortOptionRow('recent', '最近打开')
    }
    .width(168)
    .padding({ left: 6, right: 6, top: 7, bottom: 7 })
    .borderRadius(22)
    .linearGradient({
      angle: 135,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_surface_muted'), 0.64], [$r('app.color.app_primary_soft'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .shadow({
      radius: 24,
      color: '#101B2F2E',
      offsetX: 0,
      offsetY: 12
    })
  }

  @Builder
  private SortModeButton() {
    Row({ space: 6 }) {
      Text('排序')
        .fontSize(12)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.app_accent_text'))
        .maxLines(1)

      Text(this.sortModeText())
        .fontSize(12)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('sys.color.ohos_id_color_text_primary'))
        .maxLines(1)

      Image(this.sortMenuVisible ? $r('sys.media.ohos_ic_public_arrow_up') : $r('sys.media.ohos_ic_public_arrow_down'))
        .width(12)
        .height(12)
        .objectFit(ImageFit.Contain)
        .fillColor($r('app.color.app_accent_text'))
        .opacity(0.72)
    }
    .height(34)
    .padding({ left: 12, right: 10 })
    .borderRadius(17)
    .linearGradient({
      angle: 135,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .onClick(() => {
      this.sortMenuVisible = !this.sortMenuVisible
    })
    .bindPopup(this.sortMenuVisible, {
      builder: this.SortMenu(),
      placement: Placement.BottomRight,
      targetSpace: 14,
      enableArrow: false,
      autoCancel: true,
      mask: false,
      popupColor: $r('app.color.app_surface'),
      radius: 22,
      shadow: {
        radius: 34,
        color: '#101B2F45',
        offsetX: 0,
        offsetY: 18
      },
      backgroundBlurStyle: BlurStyle.Regular,
      onStateChange: (event: PopupStateChange) => {
        this.sortMenuVisible = event.isVisible
      }
    })
  }

  @Builder
  private LocalFilesHeader() {
    Column() {
      Row({ space: 10 }) {
        Column() {
          Image($r('sys.media.ohos_ic_normal_white_grid_folder'))
            .width(22)
            .height(22)
            .opacity(0.82)
        }
        .width(38)
        .height(38)
        .borderRadius(12)
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)
        .linearGradient({
          angle: 135,
          colors: [[$r('app.color.app_surface_muted'), 0.0], [$r('app.color.app_primary_soft'), 1.0]]
        })
        .border({ width: 1, color: $r('app.color.app_surface_border') })

        Column({ space: 2 }) {
          Text('本地文件')
            .fontSize(18)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
          Text(this.filterText(this.selectedFilter))
            .fontSize(12)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        this.SortModeButton()
      }
      .height(58)
      .width('100%')
      .padding({ left: 12, right: 10 })
      .alignItems(VerticalAlign.Center)

      Row() {
        Text('')
          .height(1)
          .layoutWeight(1)
          .backgroundColor($r('sys.color.ohos_id_color_list_separator'))
      }
      .width('100%')
      .padding({ left: 60 })
    }
    .width('100%')
  }

  @Builder
  private ItemCard(item: AppItemVM,index:number) {
    Column() {
      Row({ space: 10 }) {
        Column() {
          if (item.icon) {
            Image(item.icon)
              .width(30)
              .height(30)
              .objectFit(ImageFit.Contain)
              .interpolation(ImageInterpolation.Medium)
          } else {
            Column({ space: 1 }) {
              Text(this.kindMark(item.kind))
                .fontSize(9)
                .fontWeight(FontWeight.Bold)
                .fontColor(this.kindTint(item.kind))
              Row({ space: 3 }) {
                Text('')
                  .width(4)
                  .height(4)
                  .borderRadius(2)
                  .backgroundColor($r('app.color.app_icon_dot'))
                Text('')
                  .width(4)
                  .height(4)
                  .borderRadius(2)
                  .backgroundColor($r('app.color.app_icon_dot'))
                Text('')
                  .width(4)
                  .height(4)
                  .borderRadius(2)
                  .backgroundColor($r('app.color.app_icon_dot'))
              }
              .justifyContent(FlexAlign.Center)
            }
            .alignItems(HorizontalAlign.Center)
          }
        }
        .width(36)
        .height(36)
        .justifyContent(FlexAlign.Center)
        .alignItems(HorizontalAlign.Center)
        .borderRadius(10)
        .linearGradient({
          angle: 140,
          colors: [[this.kindTileStart(item.kind), 0.0], [this.kindTileEnd(item.kind), 1.0]]
        })
        .border({ width: 1, color: $r('app.color.app_surface_border') })

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
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        Column({ space: 3 }) {
          Text(item.kind === 'game' ? '游戏' : '应用')
            .fontSize(11)
            .fontColor(this.kindTint(item.kind))
            .textAlign(TextAlign.End)
            .width('100%')
            .maxLines(1)
          if (this.recentLabel(item).length > 0) {
            Text(this.recentLabel(item))
              .fontSize(10)
              .fontColor($r('sys.color.ohos_id_color_text_secondary'))
              .textAlign(TextAlign.End)
              .width('100%')
              .maxLines(1)
          }
        }
        .width(46)
        .alignItems(HorizontalAlign.End)
      }
      .width('100%')
      .height(56)
      .padding({ left: 10, right: 10 })
      .opacity(item.status === 'ready' ? 1 : 0.46)
      .alignItems(VerticalAlign.Center)
      .onClick(() => {
        if (item.status !== 'ready') {
          return
        }
        if (this.onItemTap) {
          this.onItemTap(item.id)
        }
      })
      .gesture(
        LongPressGesture({ repeat: false })
          .onAction(() => {
            if (this.onItemLongPress) {
              this.onItemLongPress(item.id)
            }
          })
      )

      Row() {
        Text('')
          .height(1)
          .layoutWeight(1)
          .backgroundColor($r('sys.color.ohos_id_color_list_separator'))
      }
      .width('100%')
      .padding({ left: 56 })
      .visibility(this.visibleItems().length-1 == index ? Visibility.None : Visibility.Visible)
    }
    .width('100%')
  }

  @Builder
  private Items() {
    if (this.visibleItems().length > 0) {
      ListItem() {
        Column() {
          if (this.showHeader) {
            this.LocalFilesHeader()
          }
          ForEach(this.visibleItems(), (item: AppItemVM,index:number) => {
            this.ItemCard(item,index)
          }, (item: AppItemVM) => item.id)
        }
        .width('100%')
        .backgroundColor($r('app.color.app_surface'))
        .borderRadius(20)
        .border({ width: 1, color: $r('app.color.app_surface_border') })
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
  }

  @Builder
  private FilterEmptyState() {
    if (this.visibleItems().length === 0) {
      ListItem() {
        Column({ space: 7 }) {
          Text(this.emptyText())
            .fontSize(16)
            .fontWeight(FontWeight.Medium)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
          Text('可以切换上方分类或修改搜索词。')
            .fontSize(12)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
        }
        .width('100%')
        .padding({ left: 20, right: 20, top: 36, bottom: 36 })
        .margin({ top: 18 })
        .alignItems(HorizontalAlign.Center)
        .borderRadius(16)
        .backgroundColor($r('app.color.app_surface'))
        .border({ width: 1, color: $r('app.color.app_surface_border') })
      }
    }
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
      ListItem() {
        Text(`${this.sortRefreshTick}-${this.activeSortMode}-${this.recentAppIdsText}`).fontSize(1).opacity(0)
      }
      .height(1)
      .selectable(false)
      this.TopSafeGap()
      if (this.showSearch) {
        this.SearchBar()
      }
      if (this.showFilters) {
        this.FilterBar()
      }
      this.Items()
      this.FilterEmptyState()
      this.BottomSafeTail()
    }
    .width('100%')
    .layoutWeight(1)
    .padding({ left: 16, right: 16, top: 0, bottom: 0 })
    .backgroundColor($r('sys.color.ohos_id_color_titlebar_bg_transparent'))
    .edgeEffect(EdgeEffect.Spring)
  }
}
