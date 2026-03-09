@Component
export struct EmptyState {
  @Prop text: string = '暂无数据'

  build() {
    Column() {
      Text(this.text)
    }
    .width('100%')
    .padding(24)
  }
}
