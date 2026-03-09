@Component
export struct LoadingBlock {
  @Prop text: string = '加载中'

  build() {
    Column() {
      LoadingProgress()
      Text(this.text).margin({ top: 8 })
    }
    .width('100%')
    .padding(16)
  }
}
