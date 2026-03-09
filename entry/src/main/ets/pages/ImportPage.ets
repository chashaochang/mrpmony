import { ImportStore } from '../store/ImportStore'
import { PageHeader } from '../components/common/PageHeader'
import { StatusBanner } from '../components/common/StatusBanner'
import { ImportSourceSelector } from '../components/import/ImportSourceSelector'
import { PackagePreviewCard } from '../components/import/PackagePreviewCard'
import { ImportProgressPanel } from '../components/import/ImportProgressPanel'
import { ImportResultPanel } from '../components/import/ImportResultPanel'

@Component
export struct ImportPage {
  onBack?: () => void
  onImportSuccess?: () => void

  @State refreshTick: number = 0
  private store: ImportStore = new ImportStore()
  private unsubscribe?: () => void

  aboutToAppear(): void {
    if (!this.unsubscribe) {
      this.unsubscribe = this.store.subscribe(() => {
        this.refreshTick += 1
      })
    }
  }

  aboutToDisappear(): void {
    if (this.unsubscribe) {
      this.unsubscribe()
      this.unsubscribe = undefined
    }
    this.store.reset()
  }

  private onBackClick(): void {
    this.store.reset()
    if (this.onBack) {
      this.onBack()
    }
  }

  private async onPickFile(): Promise<void> {
    await this.store.pickFile()
    await this.store.analyzeSelectedFile()
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

  build() {
    Column() {
      Text(`${this.refreshTick}`).fontSize(1).opacity(0)
      PageHeader({ title: '导入应用' })
      if (this.onBack) {
        Button('返回列表').onClick(() => {
          this.onBackClick()
        }).margin({ bottom: 8, left: 16, right: 16 })
      }
      if (this.store.error) {
        StatusBanner({ text: this.store.error.message })
      }
      ImportSourceSelector({ onPick: () => this.onPickFile() })
      if (this.store.previewInfo) {
        PackagePreviewCard({ name: this.store.previewInfo.name })
        Button('开始导入').onClick(() => {
          this.onImport()
        }).margin(16)
      }
      if (this.store.phase === 'importing') {
        ImportProgressPanel({ text: '导入中' })
      }
      if (this.store.phase === 'success') {
        ImportResultPanel({ text: '导入成功，正在返回列表' })
      }
      if (this.store.phase === 'failed') {
        ImportResultPanel({ text: '导入失败' })
      }
    }
    .width('100%')
    .height('100%')
  }
}
