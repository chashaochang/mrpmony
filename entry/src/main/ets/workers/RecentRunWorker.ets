import worker, { MessageEvents } from '@ohos.worker'

interface RecentWorkerRequest {
  action: string
  appId?: string
  appIdsText?: string
  currentText?: string
}

interface RecentWorkerResponse {
  action: string
  recentAppIdsText: string
}

function normalizedIds(text: string): string[] {
  if (text.length === 0) {
    return []
  }
  return text.split('|').filter((id: string) => id.length > 0)
}

function recentAfterRecord(currentText: string, appId: string): string {
  if (appId.length === 0) {
    return currentText
  }
  const ids = normalizedIds(currentText).filter((id: string) => id !== appId)
  ids.unshift(appId)
  return ids.slice(0, 20).join('|')
}

function recentAfterRemove(currentText: string, appId: string): string {
  if (appId.length === 0) {
    return currentText
  }
  return normalizedIds(currentText).filter((id: string) => id !== appId).join('|')
}

function recentAfterRemoveMany(currentText: string, appIdsText: string): string {
  if (appIdsText.length === 0) {
    return currentText
  }
  const removeMap: Record<string, boolean> = {}
  for (const appId of normalizedIds(appIdsText)) {
    removeMap[appId] = true
  }
  return normalizedIds(currentText).filter((id: string) => !removeMap[id]).join('|')
}

worker.workerPort.onmessage = (event: MessageEvents) => {
  const request = event.data as RecentWorkerRequest
  const currentText = request.currentText ?? ''
  let nextText = currentText

  if (request.action === 'record') {
    nextText = recentAfterRecord(currentText, request.appId ?? '')
  } else if (request.action === 'remove') {
    nextText = recentAfterRemove(currentText, request.appId ?? '')
  } else if (request.action === 'removeMany') {
    nextText = recentAfterRemoveMany(currentText, request.appIdsText ?? '')
  } else if (request.action === 'clear') {
    nextText = ''
  }

  const response: RecentWorkerResponse = {
    action: 'recentChanged',
    recentAppIdsText: nextText
  }
  worker.workerPort.postMessage(response)
}
