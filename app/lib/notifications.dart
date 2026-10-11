import 'package:flutter_local_notifications/flutter_local_notifications.dart';

final _plugin = FlutterLocalNotificationsPlugin();
bool _ready = false;

const _details = NotificationDetails(
  android: AndroidNotificationDetails(
    'plant_alerts',
    'Plant alerts',
    channelDescription: 'Dry soil, low tank, temperature and sensor problems',
    icon: 'ic_stat_plant',
    importance: Importance.high,
    priority: Priority.high,
  ),
);

Future<void> initNotifications() async {
  if (_ready) return;
  await _plugin.initialize(settings: const InitializationSettings(android: AndroidInitializationSettings('ic_stat_plant')));
  _ready = true;
}

/// One notification per [kind]: a repeat of the same kind replaces the previous one.
Future<void> showPlantNotification(String kind, String title, String body) async {
  await initNotifications();
  await _plugin.show(id: kind.hashCode & 0x7fffffff, title: title, body: body, notificationDetails: _details);
}
