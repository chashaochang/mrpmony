import { ImportStore } from '../store/ImportStore'
import { PageHeader } from '../components/common/PageHeader'
import { StatusBanner } from '../components/common/StatusBanner'
import { ImportSourceSelector } from '../components/import/ImportSourceSelector'
import { PackagePreviewCard } from '../components/import/PackagePreviewCard'
import { ImportProgressPanel } from '../components/import/ImportProgressPanel'
import { ImportResultPanel } from '../components/import/ImportResultPanel'

@Component
export struct ImportPage {
  private store: ImportStore = new ImportStore()

  build() {
    Column() {
      PageHeader({ title: '导入应用' })
      if (this.store.error) {
        StatusBanner({ text: this.store.error.message })
      }
      ImportSourceSelector()
      if (this.store.previewInfo) {
        PackagePreviewCard({ name: this.store.previewInfo.name })
      }
      if (this.store.phase === 'importing') {
        ImportProgressPanel({ text: '导入中' })
      }
      if (this.store.phase === 'success') {
        ImportResultPanel({ text: '导入成功' })
      }
      if (this.store.phase === 'failed') {
        ImportResultPanel({ text: '导入失败' })
      }
    }
    .width('100%')
    .height('100%')
  }
}
