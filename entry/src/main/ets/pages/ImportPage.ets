import common from '@ohos.app.ability.common'
import { ImportStore } from '../store/ImportStore'
import { StatusBanner } from '../components/common/StatusBanner'
import { ImportSourceSelector } from '../components/import/ImportSourceSelector'
import { PackagePreviewCard } from '../components/import/PackagePreviewCard'
import { ImportProgressPanel } from '../components/import/ImportProgressPanel'
import { ImportResultPanel } from '../components/import/ImportResultPanel'
import type { PackagePreviewVM } from '../models/PackagePreviewVM'

const importStore: ImportStore = new ImportStore()

@ComponentV2
export struct ImportPage {
  @Param scroller: Scroller = new Scroller()
  @Event
  onImportSuccess?: () => void

  @Local refreshTick: number = 0
  @Local phaseText: string = 'idle'
  @Local debugStatusText: string = '等待选择文件'
  @Local errorText: string = ''
  @Local previewItems: PackagePreviewVM[] = []
  @Local validPreviewCount: number = 0
  private store: ImportStore = importStore
  private unsubscribe?: () => void

  aboutToAppear(): void {
    if (!this.unsubscribe) {
      this.unsubscribe = this.store.subscribe(() => {
        this.syncFromStore()
      })
    }
    this.syncFromStore()
  }

  aboutToDisappear(): void {
    // The system file picker can temporarily hide this page. Keep the store
    // subscription alive so picker results still refresh the visible page.
  }

  private onPickFile(): void {
    const context = this.getUIContext().getHostContext() as common.UIAbilityContext
    this.store.configureContext(context)
    this.store.pickAndAnalyze()
  }

  private clearSelection(): void {
    this.store.reset()
    this.syncFromStore()
  }

  private syncFromStore(): void {
    this.phaseText = this.store.phase
    this.debugStatusText = this.store.debugStatus
    this.errorText = this.store.error ? this.store.error.message : ''
    this.previewItems = [...this.store.previewInfos]
    this.validPreviewCount = this.store.validCount()
    this.refreshTick += 1
  }

  private async onImport(): Promise<void> {
    await this.store.startImport()
    if (this.store.phase === 'success') {
      this.store.reset()
      if (this.onImportSuccess) {
        this.onImportSuccess()
      }
    }
  }

  private showStatusDetail(): boolean {
    return this.phaseText === 'importing' || this.phaseText === 'failed'
  }

  private invalidPreviewCount(): number {
    return Math.max(0, this.previewItems.length - this.validPreviewCount)
  }

  private canImport(): boolean {
    return this.validPreviewCount > 0 && this.phaseText !== 'importing' && this.phaseText !== 'picking' && this.phaseText !== 'analyzing'
  }

  @Builder
  private TopSafeGap() {
    Column()
      .width('100%')
      .height(80)
  }

  private primaryActionText(): string {
    if (this.phaseText === 'picking') {
      return '正在打开文件选择器'
    }
    if (this.phaseText === 'analyzing') {
      return '正在检查文件'
    }
    if (this.phaseText === 'importing') {
      return '正在导入'
    }
    if (this.validPreviewCount > 0) {
      return `开始导入 ${this.validPreviewCount} 个`
    }
    return '选择文件后导入'
  }

  private bottomActionHeight(): number {
    if (this.previewItems.length > 0 || this.phaseText === 'importing') {
      return 104
    }
    return 0
  }

  private estimateTimeText(): string {
    if (this.validPreviewCount <= 0) {
      return '等待文件'
    }
    const seconds = Math.max(3, Math.min(18, this.validPreviewCount * 4))
    return `约 ${seconds} 秒`
  }

  private totalSizeText(): string {
    let totalMb = 0
    let knownCount = 0
    for (const item of this.previewItems) {
      const size = item.sizeText ?? ''
      if (size.endsWith('MB')) {
        totalMb += Number(size.replace('MB', '').trim())
        knownCount += 1
      } else if (size.endsWith('KB')) {
        totalMb += Number(size.replace('KB', '').trim()) / 1024
        knownCount += 1
      }
    }
    if (knownCount === 0 || Number.isNaN(totalMb)) {
      return `${this.previewItems.length} 项`
    }
    return `${totalMb.toFixed(totalMb >= 10 ? 1 : 2)} MB`
  }

  @Builder
  private LocalSaveBanner() {
    Row({ space: 12 }) {
      Image($r('app.media.import_local_save_icon'))
        .width(40)
        .height(40)
        .objectFit(ImageFit.Contain)

      Column({ space: 5 }) {
        Text('仅在本机保存')
          .fontSize(15)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
        Text('文件仅在本机处理与保存，不会上传或共享，保障隐私安全。')
          .fontSize(11)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)
      .constraintSize({ minWidth: 0 })
    }
    .width('100%')
    .padding({ left: 14, right: 12, top: 12, bottom: 12 })
    .alignItems(VerticalAlign.Center)
    .linearGradient({
      angle: 135,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .borderRadius(22)
    .shadow({
      radius: 14,
      color: '#18384C18',
      offsetX: 0,
      offsetY: 6
    })
    .margin({ top: 10, bottom: 10 })
  }

  @Builder
  private SelectionSummary() {
    Row({ space: 12 }) {
      Column() {
        Text(`${this.previewItems.length}`)
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
          .fontColor($r('app.color.app_primary_text'))
      }
      .width(44)
      .height(44)
      .borderRadius(15)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .backgroundColor($r('app.color.app_primary_soft'))

      Column({ space: 3 }) {
        Text(`已选择 ${this.previewItems.length} 个文件`)
          .fontSize(16)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
        Text(`可导入 ${this.validPreviewCount} 个${this.invalidPreviewCount() > 0 ? `，忽略 ${this.invalidPreviewCount()} 个` : ''}`)
          .fontSize(12)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
      .layoutWeight(1)
      .alignItems(HorizontalAlign.Start)
      .constraintSize({ minWidth: 0 })

      Text('重选')
        .fontSize(13)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.app_accent_text'))
        .height(32)
        .padding({ left: 12, right: 12 })
        .borderRadius(16)
        .backgroundColor($r('app.color.app_surface_muted'))
        .border({ width: 1, color: $r('app.color.app_surface_border') })
        .onClick(() => this.onPickFile())
    }
    .width('100%')
    .padding({ left: 14, right: 12, top: 12, bottom: 12 })
    .alignItems(VerticalAlign.Center)
    .backgroundColor($r('app.color.app_surface'))
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .borderRadius(20)
    .margin({ top: 8, bottom: 10 })
  }

  @Builder
  private PreviewList() {
    Column({ space: 14 }) {
      Row() {
        Column({ space: 4 }) {
          Text(`已选择 ${this.previewItems.length} 个文件`)
            .fontSize(19)
            .fontWeight(FontWeight.Bold)
            .fontColor($r('sys.color.ohos_id_color_text_primary'))
          Text(`共 ${this.totalSizeText()}`)
            .fontSize(13)
            .fontColor($r('sys.color.ohos_id_color_text_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
        .layoutWeight(1)
        .alignItems(HorizontalAlign.Start)
        .constraintSize({ minWidth: 0 })

        Text('清除')
          .fontSize(13)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_secondary'))
          .textAlign(TextAlign.Center)
          .width(54)
          .height(34)
          .padding({ left: 10, right: 10 })
          .borderRadius(17)
          .backgroundColor($r('app.color.app_surface_muted'))
          .border({ width: 1, color: $r('app.color.app_surface_border') })
          .shadow({
            radius: 10,
            color: '#18384C14',
            offsetX: 0,
            offsetY: 5
          })
          .onClick(() => this.clearSelection())
      }
      .width('100%')
      .padding({ left: 4, right: 4 })
      .alignItems(VerticalAlign.Center)

      Column({ space: 0 }) {
        ForEach(this.previewItems, (item: PackagePreviewVM) => {
          PackagePreviewCard({
            name: item.name,
            version: item.version ?? '',
            sizeText: item.sizeText ?? '',
            valid: item.valid
          })
        }, (item: PackagePreviewVM) => item.name)
      }
      .width('100%')
      .padding({ left: 8, right: 8, top: 8, bottom: 8 })
      .borderRadius(18)
      .backgroundColor($r('app.color.app_surface_muted'))
      .border({ width: 1, color: $r('app.color.app_surface_border') })
    }
    .width('100%')
    .padding({ left: 18, right: 18, top: 18, bottom: 20 })
    .linearGradient({
      angle: 135,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 0.58], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .borderRadius(26)
    .shadow({
      radius: 18,
      color: '#18384C1C',
      offsetX: 0,
      offsetY: 8
    })
    .margin({ top: 14, bottom: 12 })
  }

  @Builder
  private ImportPlanCard() {
    Column({ space: 0 }) {
      Row() {
        Text('导入预览')
          .fontSize(19)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.ohos_id_color_text_primary'))
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 16, bottom: 8 })
      .alignItems(VerticalAlign.Center)

      this.PlanRow('目标位置', '模拟器存储/MRP')
      this.PlanRow('可导入', `${this.validPreviewCount} 个`)
      this.PlanRow('预计耗时', this.estimateTimeText())
    }
    .width('100%')
    .linearGradient({
      angle: 135,
      colors: [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .borderRadius(24)
    .shadow({
      radius: 16,
      color: '#18384C18',
      offsetX: 0,
      offsetY: 6
    })
    .margin({ top: 10, bottom: 10 })
  }

  @Builder
  private PlanRow(label: string, value: string) {
    Row({ space: 12 }) {
      Text(label)
        .fontSize(14)
        .fontColor($r('sys.color.ohos_id_color_text_secondary'))
        .layoutWeight(1)
      Text(value)
        .fontSize(14)
        .fontWeight(FontWeight.Medium)
        .fontColor(label === '可导入' ? $r('app.color.app_accent_text') : $r('sys.color.ohos_id_color_text_primary'))
        .maxLines(1)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
    }
    .width('100%')
    .height(46)
    .padding({ left: 16, right: 16 })
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private PrimaryImportButton() {
    Row() {
      Text(this.primaryActionText())
        .fontSize(17)
        .fontWeight(FontWeight.Bold)
        .fontColor(this.canImport() ? $r('app.color.app_accent_text') : $r('sys.color.ohos_id_color_text_hint'))
    }
    .enabled(this.canImport())
    .height(48)
    .width('100%')
    .justifyContent(FlexAlign.Center)
    .alignItems(VerticalAlign.Center)
    .linearGradient({
      angle: 180,
      colors: this.canImport()
        ? [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_primary_soft'), 1.0]]
        : [[$r('app.color.app_surface'), 0.0], [$r('app.color.app_surface_muted'), 1.0]]
    })
    .borderRadius(24)
    .border({ width: 1, color: $r('app.color.app_surface_border') })
    .shadow({
      radius: this.canImport() ? 14 : 0,
      color: this.canImport() ? '#24385022' : '#00000000',
      offsetX: 0,
      offsetY: 7
    })
    .onClick(() => {
      if (this.canImport()) {
        this.onImport()
      }
    })
  }

  @Builder
  private BottomActionBar() {
    if (this.bottomActionHeight() > 0) {
      Column() {
        this.PrimaryImportButton()
      }
      .width('100%')
      .padding({ left: 12, right: 12, top: 12, bottom: 12 })
      .borderRadius(32)
      .border({ width: 1, color: $r('app.color.app_surface_border') })
      .shadow({
        radius: 22,
        color: '#10182028',
        offsetX: 0,
        offsetY: 10
      })
      .margin({ left: 20, right: 20, bottom: 20 })
      .align(Alignment.Bottom)
    }
  }

  @Builder
  private BottomContentSpacer() {
    Column()
      .width('100%')
      .height(this.bottomActionHeight())
  }

  build() {
    Stack() {
      Scroll(this.scroller) {
        Column() {
          Text(`${this.refreshTick}`).fontSize(1).opacity(0)
          this.TopSafeGap()
          ImportSourceSelector({ onPick: () => this.onPickFile() })
          this.LocalSaveBanner()
          if (this.errorText) {
            StatusBanner({ text: this.errorText, tone: 'error' })
          }
          if (this.showStatusDetail()) {
            StatusBanner({ text: `状态：${this.debugStatusText}`, tone: this.phaseText === 'failed' ? 'error' : 'info' })
          }
          if (this.previewItems.length > 0) {
            this.PreviewList()
            this.ImportPlanCard()
          }
          if (this.phaseText === 'importing') {
            ImportProgressPanel({ text: '导入中' })
          }
          if (this.phaseText === 'success') {
            ImportResultPanel({ text: '导入成功，正在返回列表', tone: 'success' })
          }
          if (this.phaseText === 'failed') {
            ImportResultPanel({ text: '导入失败', tone: 'error' })
          }
          this.BottomContentSpacer()
        }
        .width('100%')
        .constraintSize({ minHeight: '100%' })
        .padding({ left: 20, right: 20 })
        .justifyContent(FlexAlign.Start)
        .alignItems(HorizontalAlign.Center)
      }
      .width('100%')
      .height('100%')
      .edgeEffect(EdgeEffect.Spring)

      this.BottomActionBar()
    }
    .width('100%')
    .height('100%')
    .linearGradient({
      angle: 180,
      colors: [
        [$r('app.color.home_background_start'), 0.0],
        [$r('app.color.home_background_mid'), 0.48],
        [$r('app.color.home_background_end'), 1.0]
      ]
    })
    .align(Alignment.Bottom)
    .expandSafeArea([SafeAreaType.SYSTEM], [SafeAreaEdge.BOTTOM])
  }
}
