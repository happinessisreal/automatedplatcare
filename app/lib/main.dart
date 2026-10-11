import 'package:flutter/material.dart';
import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'background.dart';
import 'models.dart';
import 'plant_service.dart';
import 'screens/alerts_screen.dart';
import 'screens/dashboard_screen.dart';
import 'screens/settings_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  FlutterForegroundTask.initCommunicationPort();
  initBackgroundService();
  final service = PlantService()..start();
  runApp(PlantCareApp(service: service));
}

class PlantCareApp extends StatelessWidget {
  const PlantCareApp({super.key, required this.service});

  final PlantService service;

  @override
  Widget build(BuildContext context) {
    const seed = Color(0xFF2E7D32);
    return MaterialApp(
      title: 'Plant Care',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(colorSchemeSeed: seed, brightness: Brightness.light),
      darkTheme: ThemeData(colorSchemeSeed: seed, brightness: Brightness.dark),
      home: HomePage(service: service),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key, required this.service});

  final PlantService service;

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  int _tab = 0;
  String? _shownAlertKey;
  late final AppLifecycleListener _lifecycle;

  PlantService get service => widget.service;

  @override
  void initState() {
    super.initState();
    service.addListener(_showLiveAlert);
    FlutterForegroundTask.addTaskDataCallback(_onServiceData);
    _lifecycle = AppLifecycleListener(onResume: service.reloadAlerts);
    WidgetsBinding.instance.addPostFrameCallback((_) => _setUpBackgroundAlerts());
  }

  @override
  void dispose() {
    service.removeListener(_showLiveAlert);
    FlutterForegroundTask.removeTaskDataCallback(_onServiceData);
    _lifecycle.dispose();
    super.dispose();
  }

  void _onServiceData(Object data) {
    if (data == alertsChangedMessage) service.reloadAlerts();
  }

  /// First launch: ask for notifications and battery-optimisation exemption, then start the service.
  Future<void> _setUpBackgroundAlerts() async {
    if (await FlutterForegroundTask.checkNotificationPermission() != NotificationPermission.granted) {
      await FlutterForegroundTask.requestNotificationPermission();
    }
    final prefs = SharedPreferencesAsync();
    if (await prefs.getBool('askedBattery') != true) {
      await prefs.setBool('askedBattery', true);
      if (!await FlutterForegroundTask.isIgnoringBatteryOptimizations) {
        await FlutterForegroundTask.requestIgnoreBatteryOptimization();
      }
    }
    if (await prefs.getBool('backgroundAlerts') ?? true) await startBackgroundService();
  }

  void _showLiveAlert() {
    final alert = service.liveAlert;
    if (alert == null || alert.key == _shownAlertKey || !mounted) return;
    _shownAlertKey = alert.key;
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(
      content: Text('${alertTitle(alert.type)}: ${alert.msg}'),
      action: SnackBarAction(label: 'View', onPressed: () => setState(() => _tab = 1)),
    ));
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: service,
      builder: (context, _) => Scaffold(
        body: SafeArea(
          child: IndexedStack(index: _tab, children: [
            DashboardScreen(service: service),
            AlertsScreen(service: service),
            SettingsScreen(service: service),
          ]),
        ),
        bottomNavigationBar: NavigationBar(
          selectedIndex: _tab,
          onDestinationSelected: (i) => setState(() => _tab = i),
          destinations: [
            const NavigationDestination(icon: Icon(Icons.eco_outlined), selectedIcon: Icon(Icons.eco), label: 'Plant'),
            NavigationDestination(
              icon: Badge(
                isLabelVisible: service.telemetry?.alarms.isNotEmpty ?? false,
                child: const Icon(Icons.notifications_outlined),
              ),
              selectedIcon: const Icon(Icons.notifications),
              label: 'Alerts',
            ),
            const NavigationDestination(
                icon: Icon(Icons.settings_outlined), selectedIcon: Icon(Icons.settings), label: 'Settings'),
          ],
        ),
      ),
    );
  }
}
