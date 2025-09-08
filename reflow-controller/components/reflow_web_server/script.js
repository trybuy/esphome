// Reflow Controller Dashboard JavaScript
class ReflowDashboard {
    constructor() {
        this.temperatureChart = null;
        this.updateInterval = 1000;
        this.init();
    }
    
    init() {
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.setup());
        } else {
            this.setup();
        }
    }
    
    setup() {
        this.initializeChart();
        this.startDataUpdates();
    }
    
    initializeChart() {
        this.temperatureChart = Highcharts.chart('temperatureChart', {
            chart: {
                type: 'spline',
                animation: false,
                backgroundColor: 'transparent'
            },
            title: { text: null },
            xAxis: {
                type: 'datetime',
                title: { text: 'Time' },
                dateTimeLabelFormats: {
                    second: '%H:%M:%S',
                    minute: '%H:%M',
                    hour: '%H:%M',
                    day: '%m-%d',
                    week: '%m-%d',
                    month: '%Y-%m',
                    year: '%Y'
                }
            },
            yAxis: {
                title: { text: 'Temperature (&deg;C)' },
                plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#808080'
                }]
            },
            tooltip: {
                shared: true,
                crosshairs: true,
                formatter: function() {
                    let s = '<b>Temperature</b><br/>' + Highcharts.dateFormat('%H:%M:%S', this.x);
                    this.points.forEach((point, index) => {
                        s += '<br/><span style="color: ' + point.series.color + 
                            '; font-weight: bold;">' + point.y.toFixed(1) + ' &deg;C</span>';
                    });
                    return s;
                }
            },
            legend: { 
                enabled: true,
                align: 'right',
                verticalAlign: 'top',
                layout: 'vertical',
                x: -10,
                y: 10
            },
            plotOptions: {
                spline: {
                    marker: { enabled: false },
                    lineWidth: 3,
                    color: '#e74c3c'
                }
            },
            series: [{
                name: 'Temperature',
                data: [],
                color: '#e74c3c'
            }, {
                name: 'Reflow Profile',
                data: [],
                color: '#3498db',
                dashStyle: 'dash',
                lineWidth: 2,
                marker: { enabled: true, radius: 3 }
            }],
            credits: { enabled: false }
        });
    }
    
    updateData() {
        fetch('/data')
            .then(response => response.json())
            .then(data => {
                // Update temperature chart and display
                if (data.temperature_data && data.temperature_data.length > 0) {
                    this.temperatureChart.series[0].setData(data.temperature_data, true);
                    
                    const latest = data.temperature_data[data.temperature_data.length - 1];
                    if (latest) {
                        document.getElementById('currentTemp').innerHTML = latest[1].toFixed(1) + ' &deg;C';
                    }
                }
                
                
                // Update switch status
                const btn = document.getElementById('reflowSwitchBtn');
                const stateSpan = document.getElementById('switchState');
                
                if (btn && stateSpan) {
                    if (data.switch_state) {
                        btn.classList.add('on');
                        stateSpan.textContent = 'ON';
                    } else {
                        btn.classList.remove('on');
                        stateSpan.textContent = 'OFF';
                    }
                }
            })
            .catch(error => {
                console.error('Error fetching data:', error);
            });
    }
    
    updateProfileData() {
        fetch('/profile_data')
            .then(response => response.json())
            .then(data => {
                // Update reflow profile curve
                if (data && data.length > 0) {
                    this.temperatureChart.series[1].setData(data, true);
                } else {
                    this.temperatureChart.series[1].setData([], true);
                }
            })
            .catch(error => {
                console.error('Error fetching profile data:', error);
                this.temperatureChart.series[1].setData([], true);
            });
    }
    
    startDataUpdates() {
        this.updateData();
        
        // Update both temperature and switch data together
        setInterval(() => this.updateData(), this.updateInterval);
    }
}

// Initialize dashboard and make it globally accessible
window.reflowDashboard = new ReflowDashboard();

// Switch control function
function toggleReflowSwitch() {
    const btn = document.getElementById('reflowSwitchBtn');
    const stateSpan = document.getElementById('switchState');
    const currentState = btn.classList.contains('on');
    const newState = !currentState;
    
    fetch('/switch_control', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ state: newState })
    })
    .then(response => response.json())
    .then(data => {
        if (data.state) {
            btn.classList.add('on');
            stateSpan.textContent = 'ON';
            // Fetch and display reflow profile data when turning on
            window.reflowDashboard.updateProfileData();
        } else {
            btn.classList.remove('on');
            stateSpan.textContent = 'OFF';
            // Clear reflow profile data when turning off
            window.reflowDashboard.temperatureChart.series[1].setData([], true);
        }
    })
    .catch(error => {
        console.error('Error controlling switch:', error);
    });
}
