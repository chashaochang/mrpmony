import { RuntimeSessionStore } from '../store/RuntimeSessionStore'

@Component
export struct RunPage {
  private store: RuntimeSessionStore = new RuntimeSessionStore()

  build() {
    Column() {
      Text('RunPage')
      Text(`phase: ${this.store.phase}`)
    }
    .width('100%')
    .height('100%')
  }
}
