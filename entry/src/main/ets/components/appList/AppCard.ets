@ComponentV2
export struct AppCard {
  @Param title: string = ''

  build() {
    Column() {
      Text(this.title)
    }
    .padding(16)
  }
}
