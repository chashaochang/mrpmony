import webview from '@ohos.web.webview'

@ComponentV2
export struct WebPage {
  @Param url: string = ''
  private controller: webview.WebviewController = new webview.WebviewController()

  build() {
    Column() {
      Web({ src: this.url, controller: this.controller })
        .width('100%')
        .height('100%')
        .javaScriptAccess(true)
        .domStorageAccess(true)
    }
    .width('100%')
    .height('100%')
    .padding({
      top: 74
    })
    .backgroundColor($r('sys.color.ohos_id_color_background'))
  }
}
