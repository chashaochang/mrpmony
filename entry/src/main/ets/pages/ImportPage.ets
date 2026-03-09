import { ImportStore } from '../store/ImportStore'

@Component
export struct ImportPage {
  private store: ImportStore = new ImportStore()

  build() {
    Column() {
      Text('ImportPage')
      Text(`phase: ${this.store.phase}`)
    }
    .width('100%')
    .height('100%')
  }
}
