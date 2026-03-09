@Component
export struct RuntimeToolbar {
  build() {
    Row({ space: 12 }) {
      Button('返回')
      Button('重启')
      Button('退出')
    }
    .padding(16)
  }
}
