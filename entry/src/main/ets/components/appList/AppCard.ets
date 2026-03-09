@Component
export struct AppCard {
  @Prop title: string = ''

  build() {
    Column() {
      Text(this.title)
    }
    .padding(16)
  }
}
