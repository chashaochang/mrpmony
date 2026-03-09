@Component
export struct RuntimeToolbar {
  onBack?: () => void
  onRestart?: () => void
  onPullFrame?: () => void
  onExit?: () => void
  @Prop canRestart: boolean = true
  @Prop canPullFrame: boolean = true
  @Prop canExit: boolean = true

  build() {
    Row({ space: 12 }) {
      Button('返回').onClick(() => {
        if (this.onBack) {
          this.onBack()
        }
      })
      Button('重启')
        .enabled(this.canRestart)
        .onClick(() => {
          if (this.onRestart) {
            this.onRestart()
          }
        })
      Button('拉帧')
        .enabled(this.canPullFrame)
        .onClick(() => {
          if (this.onPullFrame) {
            this.onPullFrame()
          }
        })
      Button('退出')
        .enabled(this.canExit)
        .onClick(() => {
          if (this.onExit) {
            this.onExit()
          }
        })
    }
    .padding(16)
  }
}
