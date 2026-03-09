@Component
export struct RuntimeToolbar {
  onBack?: () => void
  onRestart?: () => void
  onExit?: () => void

  build() {
    Row({ space: 12 }) {
      Button('返回').onClick(() => {
        if (this.onBack) {
          this.onBack()
        }
      })
      Button('重启').onClick(() => {
        if (this.onRestart) {
          this.onRestart()
        }
      })
      Button('退出').onClick(() => {
        if (this.onExit) {
          this.onExit()
        }
      })
    }
    .padding(16)
  }
}
