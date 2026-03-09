@Component
export struct ImportProgressPanel {
  @Prop text: string = '导入中'

  build() {
    Column() {
      LoadingProgress()
      Text(this.text).margin({ top: 8 })
    }
    .padding(16)
  }
}
