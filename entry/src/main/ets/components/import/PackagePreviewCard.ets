@Component
export struct PackagePreviewCard {
  @Prop name: string = ''

  build() {
    Column() {
      Text(this.name || '未选择文件')
    }
    .padding(16)
  }
}
